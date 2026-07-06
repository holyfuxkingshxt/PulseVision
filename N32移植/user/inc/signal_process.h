#ifndef SIGNAL_PROCESS_H
#define SIGNAL_PROCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FIR_TAPS        27
#define CODE_LEN        63
#define ENERGY_POINTS   600
#define DECIM           4
#define SOUND_SPEED     1462.0f

#define START_IDX                 720
#define MAX_ADC_USE               12095
#define ENERGY_BETA               0.03f

#define CAL_MEAN_POINTS           100
#define THRESHOLD_PEAK            80000

#define GAMMA                     1.0f
#define ALPHA                     0.047f
#define R0                        0.9f

/*
 * 找到最高峰后，往前搜索多少个“降采样后的点”
 * DECIM = 4，所以 FINE_SEARCH_SPAN = 60
 * 等价于往前搜索 60 * 4 = 240 个原始 ADC 点
 */
#define FINE_SEARCH_SPAN          60

/*
 * 相关峰 mag 缓存长度
 * MAX_ADC_USE / DECIM 大约是 3023
 */
#define MAG_BUF_LEN               ((MAX_ADC_USE / DECIM) + 4)

/*
 * 半峰比例
 * 0.5f 表示大于最高峰的 50%
 */
#define HALF_PEAK_RATIO           0.5f

/*
 * 原来的搜索起始点
 */
#define PEAK_SEARCH_MIN_IDX       100

/*
 * 原来的近场/远场分界
 */
#define PEAK_NEAR_END_IDX         160

/*
 * 近场高阈值倍率
 */
#define NEAR_PEAK_THRESHOLD_MUL   10000.0f

typedef struct
{
    int start_idx;
    int peak_idx;
    float peak_val;
    float distance_mm;
} process_result_t;

int signal_process(uint16_t* rx, int rx_len, int adc_end, process_result_t* result);

#ifdef __cplusplus
}
#endif

#endif