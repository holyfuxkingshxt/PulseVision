/*
 * signal_process_under30ms_lowram.c
 *
 * 相对于 signal_process_under30ms.c：
 * 1. 删除完整的 s_i_decim[] / s_q_decim[]，节省约 12 KB RAM；
 * 2. 仅保留 63 点 I/Q 双倍环形窗口，共 63 * 2 * 2 * 2 = 504 字节；
 * 3. 仍然采用 Q15 FIR、__SMLAD 和严格等价的滑动相关递推；
 * 4. 保留完整 mag 缓冲，峰值与半峰搜索逻辑不变。
 *
 * 适用限制：
 * FIR_TAPS == 27
 * DECIM    == 4
 * CODE_LEN == 63
 */

#include "signal_process.h"
#include <stdint.h>
#include <string.h>
#include "n32g4fr.h"
/* =========================== 可调配置 =========================== */

#define FIR_Q_SHIFT 15
#define FIR_Q_ROUND (1L << (FIR_Q_SHIFT - 1))

#ifndef CORR_SHIFT
#define CORR_SHIFT 5
#endif

/*
 * 0：THRESHOLD_PEAK 使用原浮点版本的幅值尺度，程序自动缩放；
 * 1：THRESHOLD_PEAK 已按当前定点相关幅值重新标定。
 */
#ifndef THRESHOLDS_ALREADY_SCALED
#define THRESHOLDS_ALREADY_SCALED 0
#endif

/*
 * 1：result->peak_val 恢复到接近原浮点版本的尺度；
 * 0：输出内部缩放后的幅值。
 */
#ifndef RESTORE_PEAK_VALUE_SCALE
#define RESTORE_PEAK_VALUE_SCALE 1
#endif

/* 正式测速保持为 0。 */
#ifndef SP_ENABLE_DEBUG
#define SP_ENABLE_DEBUG 0
#endif

/* =========================== 参数检查 =========================== */

#if FIR_TAPS != 27
#error "This version requires FIR_TAPS == 27"
#endif

#if DECIM != 4
#error "This version requires DECIM == 4"
#endif

#if CODE_LEN != 63
#error "This version requires CODE_LEN == 63"
#endif

#if (CORR_SHIFT < 0) || ((2 * CORR_SHIFT) >= 31)
#error "CORR_SHIFT must satisfy 0 <= CORR_SHIFT and 2*CORR_SHIFT < 31"
#endif

#define DECIMATED_POINT_COUNT ((MAX_ADC_USE + DECIM - 1) / DECIM)
#define CORR_WINDOW_COUNT     (DECIMATED_POINT_COUNT - CODE_LEN + 1)
#define CORR_MAG_SCALE_U32    (1UL << (2 * CORR_SHIFT))
#define CORR_MAG_SCALE_F      ((float)CORR_MAG_SCALE_U32)

#if DECIMATED_POINT_COUNT < CODE_LEN
#error "MAX_ADC_USE is too small"
#endif

#if MAG_BUF_LEN < CORR_WINDOW_COUNT
#error "MAG_BUF_LEN is too small for MAX_ADC_USE / DECIM / CODE_LEN"
#endif

#if SP_ENABLE_DEBUG
volatile uint32_t g_sp_max_mag_internal = 0U;
volatile int32_t  g_sp_max_mag_idx = -1;
volatile uint32_t g_sp_far_threshold_internal = 0U;
volatile uint32_t g_sp_near_threshold_internal = 0U;
volatile uint32_t g_sp_corr_window_count = 0U;
#endif

/* =========================== 静态工作区 =========================== */

/*
 * 只保留当前 63 点窗口。
 * 数据存两份，使窗口无论从哪个环形位置开始都连续可读。
 * RAM：2 路 * 126 点 * 2 字节 = 504 字节。
 */
static int16_t s_i_win2[CODE_LEN * 2];
static int16_t s_q_win2[CODE_LEN * 2];

/*
 * 保存相关幅值，供最高峰前的半峰局部峰搜索。
 * 这是原算法已经需要的缓冲，不是本版本新增的 12 KB I/Q 数组。
 */
static uint32_t s_mag_buf[MAG_BUF_LEN];

/* =========================== DSP 工具函数 =========================== */

#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE static inline
#endif

__STATIC_FORCEINLINE int32_t pack_s16(int16_t low, int16_t high)
{
    return (int32_t)(
        ((uint32_t)(uint16_t)low) |
        ((uint32_t)(uint16_t)high << 16)
    );
}

__STATIC_FORCEINLINE int32_t load_s16_pair(const int16_t *p)
{
    return (int32_t)__UNALIGNED_UINT32_READ(p);
}

#define PACK_CONST_S16(low, high) \
    ((int32_t)(                              \
        ((uint32_t)(uint16_t)(low)) |        \
        ((uint32_t)(uint16_t)(high) << 16)   \
    ))

__STATIC_FORCEINLINE int16_t sat_q15_from_acc(int32_t acc)
{
    acc = (acc + FIR_Q_ROUND) >> FIR_Q_SHIFT;
    return (int16_t)__SSAT(acc, 16);
}

__STATIC_FORCEINLINE uint32_t peak_threshold_to_internal(float threshold)
{
#if THRESHOLDS_ALREADY_SCALED
    if (threshold <= 0.0f)
    {
        return 0U;
    }

    return (uint32_t)threshold;
#else
    float scaled;

    if (threshold <= 0.0f)
    {
        return 0U;
    }

    scaled = threshold / CORR_MAG_SCALE_F;

    if (scaled < 1.0f)
    {
        return 1U;
    }

    return (uint32_t)(scaled + 0.999999f);
#endif
}

/* CORR_SHIFT=5 时，相关结果缩放后可用两个 16 位平方乘加。 */
__STATIC_FORCEINLINE uint32_t calc_mag_iq_fast(int32_t corr_i, int32_t corr_q)
{
    const int16_t ci_s = (int16_t)(corr_i >> CORR_SHIFT);
    const int16_t cq_s = (int16_t)(corr_q >> CORR_SHIFT);
    const int32_t iq_pack = pack_s16(ci_s, cq_s);

    return (uint32_t)__SMLAD(iq_pack, iq_pack, 0);
}

/* =========================== 27 阶 I/Q FIR =========================== */

static const int32_t fir_i_h_pack0 = PACK_CONST_S16(  187,  -396);
static const int32_t fir_i_h_pack1 = PACK_CONST_S16(  126,   808);
static const int32_t fir_i_h_pack2 = PACK_CONST_S16(-1615,   238);

static const int32_t fir_q_h_pack0 = PACK_CONST_S16( -149,  -375);
static const int32_t fir_q_h_pack1 = PACK_CONST_S16(  772,  -196);
static const int32_t fir_q_h_pack2 = PACK_CONST_S16(-1901,  4514);

#define FIR_I_LAST_COEF 8843
#define FIR_Q_LAST_COEF 10669

/*
 * p[0]  = rx[n - 26]
 * p[26] = rx[n]
 */
__STATIC_FORCEINLINE void fir_iq_from_rx_fast(
    const uint16_t *restrict p,
    int16_t *restrict i_out,
    int16_t *restrict q_out)
{
    int32_t acc_i = 0;
    int32_t acc_q = 0;

    int16_t t0;
    int16_t t1;
    int16_t t2;
    int16_t t3;
    int16_t t4;
    int16_t t5;
    int16_t t6;

    int16_t u0;
    int16_t u1;
    int16_t u2;
    int16_t u3;
    int16_t u4;
    int16_t u5;
    int16_t u6;

    t0 = (int16_t)((int32_t)p[26] - (int32_t)p[0]);
    t1 = (int16_t)((int32_t)p[2]  - (int32_t)p[24]);
    t2 = (int16_t)((int32_t)p[22] - (int32_t)p[4]);
    t3 = (int16_t)((int32_t)p[6]  - (int32_t)p[20]);
    t4 = (int16_t)((int32_t)p[18] - (int32_t)p[8]);
    t5 = (int16_t)((int32_t)p[10] - (int32_t)p[16]);
    t6 = (int16_t)((int32_t)p[14] - (int32_t)p[12]);

    acc_i = __SMLAD(pack_s16(t0, t1), fir_i_h_pack0, acc_i);
    acc_i = __SMLAD(pack_s16(t2, t3), fir_i_h_pack1, acc_i);
    acc_i = __SMLAD(pack_s16(t4, t5), fir_i_h_pack2, acc_i);
    acc_i += (int32_t)FIR_I_LAST_COEF * (int32_t)t6;

    u0 = (int16_t)((int32_t)p[25] + (int32_t)p[1]);
    u1 = (int16_t)(-((int32_t)p[23] + (int32_t)p[3]));
    u2 = (int16_t)((int32_t)p[21] + (int32_t)p[5]);
    u3 = (int16_t)(-((int32_t)p[19] + (int32_t)p[7]));
    u4 = (int16_t)((int32_t)p[17] + (int32_t)p[9]);
    u5 = (int16_t)(-((int32_t)p[15] + (int32_t)p[11]));
    u6 = (int16_t)p[13];

    acc_q = __SMLAD(pack_s16(u0, u1), fir_q_h_pack0, acc_q);
    acc_q = __SMLAD(pack_s16(u2, u3), fir_q_h_pack1, acc_q);
    acc_q = __SMLAD(pack_s16(u4, u5), fir_q_h_pack2, acc_q);
    acc_q += (int32_t)FIR_Q_LAST_COEF * (int32_t)u6;

    *i_out = sat_q15_from_acc(acc_i);
    *q_out = sat_q15_from_acc(acc_q);
}

/* 起始 n=0,4,...,24 时，负索引按 0 补齐。 */
__STATIC_FORCEINLINE void fir_iq_from_rx_boundary(
    const uint16_t *restrict rx,
    int n,
    int16_t *restrict i_out,
    int16_t *restrict q_out)
{
    uint16_t p[27] = {0};
    int first_valid = 26 - n;

    if (first_valid < 0)
    {
        first_valid = 0;
    }

    for (int k = first_valid; k < 27; k++)
    {
        p[k] = rx[n - 26 + k];
    }

    fir_iq_from_rx_fast(p, i_out, q_out);
}

/* =========================== 63 点初始相关 =========================== */

static const int32_t ref_code_pack[32] = {
    PACK_CONST_S16( 1,  1), PACK_CONST_S16( 1,  1),
    PACK_CONST_S16( 1, -1), PACK_CONST_S16(-1, -1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16(-1,  1),
    PACK_CONST_S16(-1,  1), PACK_CONST_S16(-1,  1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16( 1,  1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16( 1, -1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16( 1, -1),
    PACK_CONST_S16(-1,  1), PACK_CONST_S16(-1,  1),
    PACK_CONST_S16( 1, -1), PACK_CONST_S16( 1,  1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16(-1,  1),
    PACK_CONST_S16( 1,  1), PACK_CONST_S16(-1,  1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16(-1, -1),
    PACK_CONST_S16( 1,  1), PACK_CONST_S16(-1,  1),
    PACK_CONST_S16(-1,  1), PACK_CONST_S16( 1,  1),
    PACK_CONST_S16(-1, -1), PACK_CONST_S16( 1,  1),
    PACK_CONST_S16( 1,  1), PACK_CONST_S16(-1,  0)
};

#define CORR_FULL_PAIR(N)                                               \
    do                                                                  \
    {                                                                   \
        const int32_t c_pack = ref_code_pack[(N)];                      \
        const int32_t i_pack = load_s16_pair(&i_win[(N) * 2]);         \
        const int32_t q_pack = load_s16_pair(&q_win[(N) * 2]);         \
        acc_i = __SMLAD(i_pack, c_pack, acc_i);                         \
        acc_q = __SMLAD(q_pack, c_pack, acc_q);                         \
    } while (0)

__STATIC_FORCEINLINE void corr_pm1_iq_full_once(
    const int16_t *restrict i_win,
    const int16_t *restrict q_win,
    int32_t *restrict corr_i,
    int32_t *restrict corr_q)
{
    int32_t acc_i = 0;
    int32_t acc_q = 0;

    CORR_FULL_PAIR(0);  CORR_FULL_PAIR(1);
    CORR_FULL_PAIR(2);  CORR_FULL_PAIR(3);
    CORR_FULL_PAIR(4);  CORR_FULL_PAIR(5);
    CORR_FULL_PAIR(6);  CORR_FULL_PAIR(7);
    CORR_FULL_PAIR(8);  CORR_FULL_PAIR(9);
    CORR_FULL_PAIR(10); CORR_FULL_PAIR(11);
    CORR_FULL_PAIR(12); CORR_FULL_PAIR(13);
    CORR_FULL_PAIR(14); CORR_FULL_PAIR(15);
    CORR_FULL_PAIR(16); CORR_FULL_PAIR(17);
    CORR_FULL_PAIR(18); CORR_FULL_PAIR(19);
    CORR_FULL_PAIR(20); CORR_FULL_PAIR(21);
    CORR_FULL_PAIR(22); CORR_FULL_PAIR(23);
    CORR_FULL_PAIR(24); CORR_FULL_PAIR(25);
    CORR_FULL_PAIR(26); CORR_FULL_PAIR(27);
    CORR_FULL_PAIR(28); CORR_FULL_PAIR(29);
    CORR_FULL_PAIR(30); CORR_FULL_PAIR(31);

    *corr_i = acc_i;
    *corr_q = acc_q;
}

#undef CORR_FULL_PAIR

/* =========================== 严格等价滑动相关 =========================== */

#define CORR_DELTA_ADJ(P0, C0, C1)                                     \
    do                                                                  \
    {                                                                   \
        const int32_t d_pack = PACK_CONST_S16((C0), (C1));             \
        const int32_t i_pack = load_s16_pair(&i_new[(P0)]);            \
        const int32_t q_pack = load_s16_pair(&q_new[(P0)]);            \
        acc_i = __SMLAD(i_pack, d_pack, acc_i);                         \
        acc_q = __SMLAD(q_pack, d_pack, acc_q);                         \
    } while (0)

#define CORR_DELTA_NONADJ(P0, C0, P1, C1)                              \
    do                                                                  \
    {                                                                   \
        const int32_t d_pack = PACK_CONST_S16((C0), (C1));             \
        const int32_t i_pack = pack_s16(i_new[(P0)], i_new[(P1)]);     \
        const int32_t q_pack = pack_s16(q_new[(P0)], q_new[(P1)]);     \
        acc_i = __SMLAD(i_pack, d_pack, acc_i);                         \
        acc_q = __SMLAD(q_pack, d_pack, acc_q);                         \
    } while (0)

__STATIC_FORCEINLINE void corr_pm1_iq_slide_exact(
    const int16_t *restrict i_new,
    const int16_t *restrict q_new,
    int16_t old_i,
    int16_t old_q,
    int32_t *restrict corr_i,
    int32_t *restrict corr_q)
{
    int32_t acc_i = *corr_i - (int32_t)old_i;
    int32_t acc_q = *corr_q - (int32_t)old_q;

    CORR_DELTA_ADJ(10, -2,  2);
    CORR_DELTA_ADJ(12, -2,  2);
    CORR_DELTA_ADJ(14, -2,  2);
    CORR_DELTA_ADJ(21, -2,  2);
    CORR_DELTA_ADJ(25, -2,  2);
    CORR_DELTA_ADJ(29,  2, -2);
    CORR_DELTA_ADJ(32,  2, -2);
    CORR_DELTA_ADJ(42, -2,  2);
    CORR_DELTA_ADJ(49,  2, -2);
    CORR_DELTA_ADJ(51,  2, -2);
    CORR_DELTA_ADJ(61,  2, -1);

    CORR_DELTA_NONADJ( 4,  2, 17, -2);
    CORR_DELTA_NONADJ(19,  2, 28, -2);
    CORR_DELTA_NONADJ(35,  2, 38, -2);
    CORR_DELTA_NONADJ(41,  2, 47, -2);
    CORR_DELTA_NONADJ(55,  2, 57, -2);

    *corr_i = acc_i;
    *corr_q = acc_q;
}

#undef CORR_DELTA_ADJ
#undef CORR_DELTA_NONADJ

/* =========================== 主处理函数 =========================== */

int signal_process(
    uint16_t *rx,
    int rx_len,
    int adc_end,
    process_result_t *result)
{
    int n = 0;
    int dec_count = 0;
    int win_pos = 0;
    int win_count = 0;
    int corr_started = 0;

    int32_t corr_i = 0;
    int32_t corr_q = 0;

    uint32_t peak_val = 0U;
    int peak_idx_local0 = -1;

    int first_peak_idx = -1;
    uint32_t first_peak_val = 0U;

    int search_first = PEAK_SEARCH_MIN_IDX;
    int search_last;

    const uint32_t far_threshold =
        peak_threshold_to_internal((float)THRESHOLD_PEAK);

    const uint32_t near_threshold =
        peak_threshold_to_internal(
            (float)THRESHOLD_PEAK * (float)NEAR_PEAK_THRESHOLD_MUL
        );

    (void)adc_end;

    if ((rx == 0) || (result == 0))
    {
        return -1;
    }

    if (rx_len < MAX_ADC_USE)
    {
        return -2;
    }

    if (search_first < 0)
    {
        search_first = 0;
    }

#if SP_ENABLE_DEBUG
    g_sp_max_mag_internal = 0U;
    g_sp_max_mag_idx = -1;
    g_sp_far_threshold_internal = far_threshold;
    g_sp_near_threshold_internal = near_threshold;
    g_sp_corr_window_count = 0U;
#endif

    /*
     * FIR 生成、63 点窗口维护和滑动相关在一个循环中完成。
     * 不再保存全部降采样 I/Q 数据。
     */
    for (n = 0; n < MAX_ADC_USE; n += DECIM)
    {
        int16_t i_out;
        int16_t q_out;
        int16_t old_i = 0;
        int16_t old_q = 0;
        int was_full;

        if (n < 28)
        {
            fir_iq_from_rx_boundary(rx, n, &i_out, &q_out);
        }
        else
        {
            fir_iq_from_rx_fast(&rx[n - 26], &i_out, &q_out);
        }

        was_full = (win_count == CODE_LEN);

        if (was_full)
        {
            old_i = s_i_win2[win_pos];
            old_q = s_q_win2[win_pos];
        }

        s_i_win2[win_pos] = i_out;
        s_q_win2[win_pos] = q_out;
        s_i_win2[win_pos + CODE_LEN] = i_out;
        s_q_win2[win_pos + CODE_LEN] = q_out;

        win_pos++;
        if (win_pos >= CODE_LEN)
        {
            win_pos = 0;
        }

        if (win_count < CODE_LEN)
        {
            win_count++;
        }

        dec_count++;

        if (win_count == CODE_LEN)
        {
            const int cur_idx = dec_count - CODE_LEN;

            if (cur_idx >= search_first)
            {
                uint32_t mag;
                const int16_t *const i_window = &s_i_win2[win_pos];
                const int16_t *const q_window = &s_q_win2[win_pos];

                if (!corr_started)
                {
                    corr_pm1_iq_full_once(
                        i_window,
                        q_window,
                        &corr_i,
                        &corr_q
                    );
                    corr_started = 1;
                }
                else
                {
                    corr_pm1_iq_slide_exact(
                        i_window,
                        q_window,
                        old_i,
                        old_q,
                        &corr_i,
                        &corr_q
                    );
                }

                mag = calc_mag_iq_fast(corr_i, corr_q);
                s_mag_buf[cur_idx] = mag;

#if SP_ENABLE_DEBUG
                g_sp_corr_window_count++;
                if (mag > g_sp_max_mag_internal)
                {
                    g_sp_max_mag_internal = mag;
                    g_sp_max_mag_idx = cur_idx;
                }
#endif

                if (cur_idx < PEAK_NEAR_END_IDX)
                {
                    if ((mag > near_threshold) && (mag > peak_val))
                    {
                        peak_val = mag;
                        peak_idx_local0 = cur_idx;
                    }
                }
                else
                {
                    if ((mag > far_threshold) && (mag > peak_val))
                    {
                        peak_val = mag;
                        peak_idx_local0 = cur_idx;
                    }
                }
            }
        }
    }

    if (dec_count < CODE_LEN)
    {
        return -3;
    }

    search_last = dec_count - CODE_LEN;

    if (!corr_started)
    {
        result->start_idx = 0;
        result->peak_idx = -1;
        result->peak_val = 0;
        result->distance_mm = 0xFFFF;
        return 0;
    }

    /* 保持原来的最高峰前半峰局部峰搜索。 */
    if (peak_idx_local0 >= 0)
    {
        int local_start = peak_idx_local0 - FINE_SEARCH_SPAN;
        int local_end = peak_idx_local0 - 1;
        const uint32_t half_peak_threshold =
            (uint32_t)((float)peak_val * (float)HALF_PEAK_RATIO);

        if (local_start < search_first)
        {
            local_start = search_first;
        }

        if (local_end > search_last - 1)
        {
            local_end = search_last - 1;
        }

        if ((local_end - local_start) >= 2)
        {
            for (int i = local_start + 1; i <= local_end - 1; i++)
            {
                if ((s_mag_buf[i] > half_peak_threshold) &&
                    (s_mag_buf[i] >= s_mag_buf[i - 1]) &&
                    (s_mag_buf[i] >= s_mag_buf[i + 1]))
                {
                    first_peak_idx = i;
                    first_peak_val = s_mag_buf[i];
                    break;
                }
            }
        }

        if (first_peak_idx >= 0)
        {
            peak_idx_local0 = first_peak_idx;
            peak_val = first_peak_val;
        }
    }

    result->start_idx = 0;
    result->peak_idx = peak_idx_local0;

#if RESTORE_PEAK_VALUE_SCALE
    result->peak_val = (float)peak_val * CORR_MAG_SCALE_F;
#else
    result->peak_val = peak_val;
#endif

    if (peak_idx_local0 < 0)
    {
        result->distance_mm = 0xFFFF;
        return 0;
    }

    result->distance_mm =
        (((float)peak_idx_local0 + 32.0f - 75.0f) / 1000.0f)
        * SOUND_SPEED
        * 0.5f;

    if ((result->distance_mm <= 0.0f) ||
        (result->distance_mm > 2300.0f))
    {
        result->distance_mm = 0xFFFF;
    }
    else if (result->distance_mm <= 50.0f)
    {
        result->distance_mm = 0x0032;
    }

    return 0;
}
