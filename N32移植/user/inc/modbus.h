#ifndef __MODBUS_H__
#define __MODBUS_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usart.h>
#define MB_FUNC_READ_HOLDING         0x03    /* ?????? */
#define MB_FUNC_WRITE_SINGLE_REG     0x06    /* ?????? */

#define REG_DISTANCE_SAVE   0x0100
#define REG_DISTANCE_REAL   0x0101
#define REG_CHANGE_BAUD	    0x0201
#define REG_CHANGE_ANGLE    0x0208
#define REG_CHANGE_NOSIY    0x021A
#define REG_TRANSMIT_ADC		0x0222
#define REG_CHANGE_SLAVE		0x2000


#define SLAVE_ADDR					0x01
#define USE_UART						0x00
extern uint8_t slave_addr;
void Modbus_Process(uint8_t rx_buf[]);
//usart1_dma_send_u16(ADC_buff,ADC_BUFF_COUNT) 

#endif