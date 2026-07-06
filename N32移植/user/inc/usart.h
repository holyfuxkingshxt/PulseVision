#ifndef __USART_H__
#define __USART_H__

#include <stdint.h>
#include "n32g4fr_usart.h"
#include "n32g4fr_dma.h"
#include "n32g4fr_gpio.h"
#include "n32g4fr_dma.h"
#include "misc.h"

#define ADC_BUFF_COUNT   12096U

void usart1_init(void);
void usart1_gpio_init(void);
void usart1_dma_tx_init(void);
void usart1_send_byte(uint8_t data);
ErrorStatus usart1_dma_send_u16(const uint16_t *buffer,uint16_t sample_count);
uint8_t usart1_dma_tx_status(void);
void usart1_dma_tx_init(void);
ErrorStatus USART1_Transmit(const uint8_t *data,uint16_t length);
extern uint16_t ADC_buff[ADC_BUFF_COUNT];
ErrorStatus USART1_SetBaudRate(uint32_t baudrate);


#endif