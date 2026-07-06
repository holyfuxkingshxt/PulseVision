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
 * @file main.c
 * @author Nations
 * @version v1.0.2
 *
 * @copyright Copyright (c) 2019, Nations Technologies Inc. All rights reserved.
 */
#include "main.h"
#include "n32g4fr.h"
#include <stdio.h>
#include <stdint.h>
#include "rcc.h"
#include "usart.h"
#include "modbus.h"
#include "signal_process.h"
/**
 * @brief  Inserts a delay time.
 * @param count specifies the delay time length.
 */
void Delay(uint32_t count)
{
    for (; count > 0; count--)
        ;
}	
/**
 * @brief Assert failed function by user.
 * @param file The name of the call that failed.
 * @param line The source line number of the call that failed.
 */

#ifdef USE_FULL_ASSERT
void assert_failed(const uint8_t* expr, const uint8_t* file, uint32_t line)
{
    while (1)
    {
    }
}
#endif // USE_FULL_ASSERT
// user define start
#define ADC_BUFF_COUNT   12096U

// user define end


//user define parameter start

// ADC采样数据存放寄存器
uint16_t ADC_buff[ADC_BUFF_COUNT] = {0};
volatile uint8_t usart1_send_request_flag = 0;
uint8_t  uart1_rx_buf[8] = {0};  
uint8_t uart1_rx_ready = 0;
//modbus
process_result_t res;        // ???????
uint16_t distance_mm = 0;    // ??????

//user define parameter end

/**
 * @brief  Main program.
 */
int main(void)
{
    /*SystemInit() function has been called by startup file startup_n32g4fr.s*/
	// RCC_Configuration();
	usart1_init();

	while (1)
	{
		/* USART1收到字符'1'后会进入这里 */
		(void)usart1_dma_tx_status();
		
		if (uart1_rx_ready == 1U)
		{
			Modbus_Process(uart1_rx_buf);
			uart1_rx_ready = 0;
		}
	}
	
}
/**
 * @}
 */
