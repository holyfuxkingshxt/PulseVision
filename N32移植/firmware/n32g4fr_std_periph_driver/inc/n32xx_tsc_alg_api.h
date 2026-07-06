/*****************************************************************************
 * Copyright (c) 2019, Nations Technologies Inc.
 *
 * All rights reserved.
 * ****************************************************************************
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the disclaimer below.
 *
 * Nations' name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * DISCLAIMER: THIS SOFTWARE IS PROVIDED BY NATIONS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * DISCLAIMED. IN NO EVENT SHALL NATIONS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * ****************************************************************************/

/**
 * @file n32xx_tsc_alg_api.h
 * @author Nations
 * @version v1.0.1
 *
 * @copyright Copyright (c) 2019, Nations Technologies Inc. All rights reserved.
 */
#ifndef __N32XX_TSC_ALG_API__
#define __N32XX_TSC_ALG_API__

#ifdef __cplusplus
extern "C" {
#endif // defined __cplusplus

#define TSC_ALG_VERSION_NUMBER "Ver1.0.4" 

#define TSC_ALG_HANDLER_STOP2_DATA_SIZE (144)
#define TSC_ALG_HANDLER_PERIOD_PER_CHN  (400)
#define TSC_ALG_WAKEUP_TIMES            (1000
#define TSC_ALG_DEBUG_BUF_SIZE          (260)
#define TSC_ALG_REF_SIZE_PER_CHN        (430)

/**
 * @brief Define touch type
 */
typedef enum tsc_alg_type_e
{
    TSC_ALG_BUTTON_TYPE = 0, ///< tsc application of simple button
    TSC_ALG_TYPE_MAX         ///<
} tsc_alg_type;

/**
 * @brief key event
 */
typedef enum tsc_press_key_event_e
{
    TSC_PRESS_KEY_NORMAL = 0, //nornal key press
    TSC_PRESS_KEY_MAX         //
} tsc_press_key_event;

/**
 * @brief Error type definition
 */
typedef enum tsc_ret_code_e
{
    TSC_SUCCESS = 0,               
    TSC_NOT_INIT_ERR,              
    TSC_NOT_REG_CHN_ERR,           
    TSC_NOT_ACCORD_LIB_ERR,        
    TSC_POINTER_NULL_ERR,          
    TSC_PARAM_ZERO_ERR,            
    TSC_REPEAT_REG_ERR,            
    TSC_CHN_NUM_ERR,               
    TSC_REG_CHANNEL_ENOUGH_ERR,    
    TSC_REG_TIMX_ERR,              
    TSC_REG_DMA_ERR,               
    TSC_SOURCE_NOT_ENOUGH_ERR,     
    TSC_NOT_SUPPORT_ERR,           
    TSC_LEVEL_CFG_ERR,             
    TSC_AUTO_CALIB_TIMER_ERR,      
    TSC_DISTURB_ERR,               
    TSC_CHN_RAM_NOT_ENOUGH_ERR,    
    TSC_STOP2_NULL_OR_INVALID_ERR, 
    TSC_DEBUG_BUF_ENOUGH_ERR       
} tsc_ret_code;

typedef enum tsc_hld_lev_e
{
    TSC_HOLD_LEV1  = 1,  // HOLD level 1(5ms)
    TSC_HOLD_LEV2  = 2,  // HOLD level 2(7ms)
    TSC_HOLD_LEV3  = 3,  // HOLD level 3(11ms)
    TSC_HOLD_LEV4  = 4,  // HOLD level 4(17ms)
    TSC_HOLD_LEV5  = 5,  // HOLD level 5(25ms)
    TSC_HOLD_LEV6  = 6,  // HOLD level 6(35ms)
    TSC_HOLD_LEV7  = 7,  // HOLD level 7(47ms)
    TSC_HOLD_LEV8  = 8,  // HOLD level 8(61ms)
    TSC_HOLD_LEV9  = 9,  // HOLD level 9(77ms)
    TSC_HOLD_LEV10 = 10, // HOLD level 10(95ms)
    TSC_HOLD_LEV11 = 11, // HOLD level 11(115ms)
    TSC_HOLD_LEV12 = 12, // HOLD level 12(137ms)
    TSC_HOLD_LEV13 = 13, // HOLD level 13(161ms)
    TSC_HOLD_LEV14 = 14, // HOLD level 14(187ms)
    TSC_HOLD_LEV15 = 15, // HOLD level 15(215ms)
    TSC_HOLD_LEV16 = 16, // HOLD level 16(245ms)
    TSC_HOLD_LEV17 = 17, // HOLD level 17(277ms)
    TSC_HOLD_LEV18 = 18, // HOLD level 18(311ms)
    TSC_HOLD_LEV19 = 19, // HOLD level 19(347ms)
    TSC_HOLD_LEV20 = 20, // HOLD level 20(385ms)
    TSC_HOLD_MAX         // Invalid
} tsc_hld_lev;

typedef enum tsc_delta_limit_lev_e
{
    TSC_DELTA_LIMIT_LEV1  = 1, 
    TSC_DELTA_LIMIT_LEV2  = 2, 
    TSC_DELTA_LIMIT_LEV3  = 3, 
    TSC_DELTA_LIMIT_LEV4  = 4, 
    TSC_DELTA_LIMIT_LEV5  = 5, 
    TSC_DELTA_LIMIT_LEV6  = 6, 
    TSC_DELTA_LIMIT_LEV7  = 7, 
    TSC_DELTA_LIMIT_LEV8  = 8, 
    TSC_DELTA_LIMIT_LEV9  = 9, 
    TSC_DELTA_LIMIT_LEV10 = 10,
    TSC_DELTA_LIMIT_LEV11 = 11,
    TSC_DELTA_LIMIT_LEV12 = 12,
    TSC_DELTA_LIMIT_LEV13 = 13,
    TSC_DELTA_LIMIT_LEV14 = 14,
    TSC_DELTA_LIMIT_LEV15 = 15,
    TSC_DELTA_LIMIT_LEV16 = 16,
    TSC_DELTA_LIMIT_LEV17 = 17,
    TSC_DELTA_LIMIT_LEV18 = 18,
    TSC_DELTA_LIMIT_LEV19 = 19,
    TSC_DELTA_LIMIT_LEV20 = 20,
    TSC_DELTA_LIMIT_MAX         // Invalid
} tsc_delta_limit_lev;


typedef enum tsc_resist_disturb_lev_e
{
    TSC_RESIST_DIS_LEV0 = 0, // default level
    TSC_RESIST_DIS_LEV1 = 1, // Enhance level
    TSC_RESIST_DIS_LEV2 = 2, // Reserved
    TSC_RESIST_DIS_MAX       // Invalid
} tsc_resist_disturb_lev;

typedef struct TSC_AlgInitThreValue_t
{
    uint16_t hold_level;     
    uint16_t rate_of_change; 
    uint32_t chn;            
} TSC_AlgInitThreValue;

/**
 * @brief TSC initialize param
 */
typedef struct TSC_AlgInitTypeDef_t
{
    TIM_Module* TIMx;                      
    DMA_ChannelType* DMAyChx;              
    uint32_t DMARemapEnable;               
    TSC_AlgInitThreValue* pTScChannelList; 
    uint32_t AutoCalibrateTimer;           
    uint32_t ResistDisturbLev;             
    uint8_t* pTscSramAddr;                 
    uint32_t TscSramSize;                  
    uint16_t* LogBuf;                      
    uint16_t LogBufSize;                   
    uint8_t* Stop2Data;                    
    uint16_t Stop2DataSize;                
} TSC_AlgInitTypeDef;

/**
 * @brief TSC algorithm real-time analysis processing(Must be placed in the TIMER interrupt function)
 * @param void
 * @return void
 */
void tsc_alg_analyze_handler(void);

/**
 * @brief Lower power calibration
 * @param uint32_t delta_limit_level
 * @uint32_t hse_or_hsi 0:HSI,  1:HSE;
 * @return
 * - `TSC_SUCCESS: Success
 * - Other value: Error
 * - Note:Used for timing calibration in STOP2 mode
 */
int32_t tsc_alg_set_powerdown_calibrate(tsc_delta_limit_lev delta_limit_level, uint32_t hse_or_hsi);

/**
 * @brief In lower power mode, check it is wakeup by disturb
 * @param void
 * @return 0:Normal wakeup; 1:Disturb wakeup
 */
int32_t tsc_alg_wakeup_disturb_check(uint32_t* wakeup_src);

/**
 * @brief Get algorithm version
 * @param void
 * @return void
 */
char* tsc_alg_get_version(void);

/**
 * @brief TSC tick ,1ms in default
 * @param void
 * @return void
 */
void tsc_alg_tick_count(void);

/**
 * @brief Get SRAM size required for the TSC algorithm 
 * uint32_t      chn_totals; // TSC touch channel number
 * @return
 * - 0:    Failed
 * - Not 0:  Succeed
 */
uint32_t tsc_alg_need_sramsize(uint32_t chn_totals);

/**
 * @brief TSC algorithm initialization
 * @param tsc_init_parameter *ptsc_init_parameter 
 * @param void
 * @return
 * - TSC_SUCCESS:Succeed
 * - Other value: Error
 */
int32_t tsc_alg_init(TSC_AlgInitTypeDef* TSC_AlgInitStruct);

/**
 * @brief TSC start work
 * @param void
 * @return
 * - TSC_SUCCESS:Succeed
 * - Other value: Error
 */
int32_t tsc_alg_start(void);

/**
 * @brief Set TSC enter low power mode(used for wakeup from low power mode)
 * @param uint32_t TScChannelList Reserved.0: all channel have been registered
 * @return
 * - TSC_SUCCESS:Succeed
 * - Other value: Error
 * - Note:Used in STOP2 mode.
 */
int32_t tsc_alg_set_powerdown(uint32_t TscChannelList);

/********************************************************
 * @brief Touch callback
 * @param tsc_touch_type
 * @param uint32_t event 0:Normal touch event
 * @param uint32_t chn: touch channel number;
 * @param uint32_t value: touch status:1 press;0 release;
 * @return
 * - TSC_SUCCESS:Succeed
 * - Other value: Error
 ********************************************************/
int32_t tsc_alg_isr_callback(tsc_alg_type type, uint32_t event, uint32_t chn, uint32_t value);


void tsc_alg_debug_output(uint32_t chn, uint8_t data);

#ifdef __cplusplus
}
#endif // defined __cplusplus

#endif //__N32XX_TSC_ALG_API__
