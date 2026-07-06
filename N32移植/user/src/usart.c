#include "usart.h"

#define GPIO_USART_TX GPIO_PIN_9
#define GPIO_USART_RX GPIO_PIN_10

#define USART1_DMA_TX_CH DMA1_CH4
#define USART1_DMA_RX_CH DMA1_CH5

#define USART1_DMA_MAX_LEN 65535U

GPIO_InitType use_gpio_usart;
USART_InitType huart1;

DMA_InitType dma_usart_tx;
DMA_InitType dma_usart_rx;

static void usart1_rx_irq_init(void);

extern volatile uint8_t usart1_send_request_flag;
static volatile uint8_t usart1_dma_busy = 0;
extern uint8_t  uart1_rx_buf[8];  
extern 	uint8_t uart1_rx_ready;
volatile uint8_t uart1_rx_index = 0;


// 配置初始化usart
void usart1_init(){
	
	usart1_gpio_init();
	huart1.BaudRate = 115200;
	huart1.WordLength = USART_WL_8B;
	huart1.StopBits = USART_STPB_1;
	huart1.Parity = USART_PE_NO;
	huart1.HardwareFlowControl = USART_HFCTRL_NONE;
	huart1.Mode = USART_MODE_RX|USART_MODE_TX;
	
	USART_Init(USART1, &huart1);
	
	usart1_dma_tx_init();
	usart1_rx_irq_init();
	
    USART_Enable(USART1, ENABLE);
}

// 配置初始化usart用到的gpio
void usart1_gpio_init(){
	RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOA|RCC_APB2_PERIPH_USART1,ENABLE);
	/* PA9：USART1_TX */
    use_gpio_usart.Pin        = GPIO_USART_TX;
    use_gpio_usart.GPIO_Mode  = GPIO_Mode_AF_PP;
    use_gpio_usart.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitPeripheral(GPIOA, &use_gpio_usart);

    /* PA10：USART1_RX */
    use_gpio_usart.Pin        = GPIO_USART_RX;
    use_gpio_usart.GPIO_Mode  = GPIO_Mode_AF_PP;
    use_gpio_usart.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitPeripheral(GPIOA, &use_gpio_usart);
}

//// 配置初始化usart用到的dma
void usart1_dma_tx_init(){
	RCC_EnableAHBPeriphClk(RCC_AHB_PERIPH_DMA1, ENABLE);
	
	DMA_EnableChannel(USART1_DMA_TX_CH, DISABLE);
    DMA_DeInit(USART1_DMA_TX_CH);
	
	/* 将USART1_TX请求连接到DMA1通道4 */
    DMA_RequestRemap(DMA1_REMAP_USART1_TX,
                     DMA1,
                     USART1_DMA_TX_CH,
                     ENABLE);

    /* 允许USART1产生发送DMA请求 */
    USART_EnableDMA(USART1, USART_DMAREQ_TX, ENABLE);
	
}

/*
 * 发送uint16_t数组。
 *
 * USART为8位数据，因此DMA将uint16_t数组按照字节发送：
 * 低字节、高字节、低字节、高字节……
 */
ErrorStatus usart1_dma_send_u16(const uint16_t *buffer,
                                uint16_t sample_count)
{
    uint32_t byte_count;

    if ((buffer == 0) || (sample_count == 0))
        return ERROR;
	
    if (usart1_dma_busy != 0)
        return ERROR;
	
    byte_count = (uint32_t)sample_count * sizeof(uint16_t);

    if (byte_count > USART1_DMA_MAX_LEN)
        return ERROR;

    /*
     * TXNUM不为0，说明上一次发送仍未完成。
     */
    if (DMA_GetCurrDataCounter(USART1_DMA_TX_CH) != 0U)
        return ERROR;

    DMA_EnableChannel(USART1_DMA_TX_CH, DISABLE);
    DMA_DeInit(USART1_DMA_TX_CH);

    dma_usart_tx.PeriphAddr     = (uint32_t)&USART1->DAT;
    dma_usart_tx.MemAddr        = (uint32_t)buffer;
    dma_usart_tx.Direction      = DMA_DIR_PERIPH_DST;
    dma_usart_tx.BufSize        = byte_count;
    dma_usart_tx.PeriphInc      = DMA_PERIPH_INC_DISABLE;
    dma_usart_tx.DMA_MemoryInc  = DMA_MEM_INC_ENABLE;

    /*
     * 使用BYTE宽度，发送uint16_t的全部两个字节。
     */
    dma_usart_tx.PeriphDataSize = DMA_PERIPH_DATA_SIZE_BYTE;
    dma_usart_tx.MemDataSize    = DMA_MemoryDataSize_Byte;

    dma_usart_tx.CircularMode   = DMA_MODE_NORMAL;
    dma_usart_tx.Priority       = DMA_PRIORITY_HIGH;
    dma_usart_tx.Mem2Mem        = DMA_M2M_DISABLE;

    DMA_Init(USART1_DMA_TX_CH, &dma_usart_tx);

    DMA_RequestRemap(DMA1_REMAP_USART1_TX,
                     DMA1,
                     USART1_DMA_TX_CH,
                     ENABLE);

    DMA_ClearFlag(DMA1_FLAG_TC4, DMA1);
    DMA_ClearFlag(DMA1_FLAG_TE4, DMA1);

	usart1_dma_busy = 1;
    DMA_EnableChannel(USART1_DMA_TX_CH, ENABLE);

    return SUCCESS;
}

/*
 * 返回值：
 * 0：正在发送
 * 1：发送完成
 * 2：DMA错误
 */
uint8_t usart1_dma_tx_status(void)
{
    if (DMA_GetFlagStatus(DMA1_FLAG_TE4, DMA1) == SET)
    {
        DMA_EnableChannel(USART1_DMA_TX_CH, DISABLE);

        DMA_ClearFlag(DMA1_FLAG_TE4, DMA1);
        DMA_ClearFlag(DMA1_FLAG_TC4, DMA1);

        usart1_dma_busy = 0;
        return 2;
    }

    if (DMA_GetFlagStatus(DMA1_FLAG_TC4, DMA1) == SET)
    {
        if (USART_GetFlagStatus(USART1,
                                USART_FLAG_TXC) == SET)
        {
            DMA_EnableChannel(USART1_DMA_TX_CH, DISABLE);
            DMA_ClearFlag(DMA1_FLAG_TC4, DMA1);

            usart1_dma_busy = 0;
            return 1;
        }
    }

    return 0;
}


void usart1_send_byte(uint8_t data)
{
    while (USART_GetFlagStatus(USART1,USART_FLAG_TXDE) == RESET)
    {
    }

    USART_SendData(USART1, data);
}


 //usart1 中断初始化

static void usart1_rx_irq_init(void)
{
    NVIC_InitType nvic_usart;

    nvic_usart.NVIC_IRQChannel                   = USART1_IRQn;
    nvic_usart.NVIC_IRQChannelPreemptionPriority = 1;
    nvic_usart.NVIC_IRQChannelSubPriority        = 0;
    nvic_usart.NVIC_IRQChannelCmd                = ENABLE;

    NVIC_Init(&nvic_usart);

    USART_ConfigInt(USART1,USART_INT_RXDNE,ENABLE);
}

// 中断处理函数
void USART1_IRQHandler(void)
{
	uint8_t received_data  = 0;

    if (USART_GetIntStatus(USART1,USART_INT_RXDNE) != RESET)
    {	
		
        received_data = (uint8_t)USART_ReceiveData(USART1);
        uart1_rx_buf[uart1_rx_index] = received_data;
		
        if(uart1_rx_ready == 0U){
			
			uart1_rx_buf[uart1_rx_index] = received_data;
			uart1_rx_index++;
			
			if(uart1_rx_index >= 8U){
				uart1_rx_index = 0;
                uart1_rx_ready = 1;
			}
		
		}
    }
}

ErrorStatus USART1_Transmit(const uint8_t *data,uint16_t length)
{
    uint16_t index;

    if ((data == 0) || (length == 0U)) return ERROR;

    for (index = 0; index < length; index++)
    {
        /* 等待发送数据寄存器为空 */
        while (USART_GetFlagStatus(USART1,USART_FLAG_TXDE) == RESET)
        {
        }
		USART_SendData(USART1, data[index]);
    }

    /* 等待最后一个字节完全发送 */
    while (USART_GetFlagStatus(USART1,USART_FLAG_TXC) == RESET)
    {
    }

    return SUCCESS;
}


ErrorStatus USART1_SetBaudRate(uint32_t baudrate)
{
    if (baudrate == 0U)
    {
        return ERROR;
    }

    /* 等待最后一个字节发送完毕 */
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXC) == RESET)
    {
    }

    /* 修改期间暂时关闭接收中断和USART */
    USART_ConfigInt(USART1, USART_INT_RXDNE, DISABLE);
    USART_Enable(USART1, DISABLE);

    huart1.BaudRate = baudrate;
    USART_Init(USART1, &huart1);

    USART_Enable(USART1, ENABLE);
    USART_ConfigInt(USART1, USART_INT_RXDNE, ENABLE);

    return SUCCESS;
}









