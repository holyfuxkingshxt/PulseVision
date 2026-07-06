#include "modbus.h"
#include "signal_process.h"

static void Modbus_SendReadHoldingOneReg(uint16_t value);
static void Modbus_SendWriteSingleRegAck(uint16_t reg_addr, uint16_t value);
static void Modbus_SendDistance(uint16_t distance_mm);
static void Modbus_SendException(uint8_t func_code, uint8_t exception_code);
static uint16_t CRC16(uint8_t *buffer, uint16_t buffer_length);
//void Send_ADC_MinCheck(void);

extern uint8_t AGC_SET;
extern process_result_t res;
extern uint16_t distance_mm;
uint16_t distance_temp = 0x0000;
extern uint8_t uart1_rx_buf[8];
extern uint16_t ADC_buff[12096];
uint8_t slave_addr = SLAVE_ADDR;

void Modbus_Process(uint8_t rx_buf[]) {
	//uint8_t req_addr;
    uint16_t received_crc;
    uint16_t calculated_crc;
    uint8_t func_code;
    uint16_t start_addr, quantity;
	uint16_t rx_len = 8;
	
	if (!rx_buf) return; /* */
	
    if (rx_buf[0] != slave_addr && rx_buf[0] != 0xFF) return;
		
    if (rx_len < 8) {
        return; 
    }
	
	received_crc = (rx_buf[rx_len - 1] << 8) | rx_buf[rx_len - 2];
    calculated_crc = CRC16(rx_buf, rx_len - 2);
		
    if (received_crc != calculated_crc) {
        return;  /* CRC?? */
    }
	
	func_code = rx_buf[1];
		switch (func_code) 
		{
			case MB_FUNC_READ_HOLDING: 
			{
						start_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];// 0x0100
						quantity   = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];// 0x0001
						if (quantity != 1)
						{		
								Modbus_SendException(func_code, 0x03);
								break;
						}
						if (start_addr == REG_DISTANCE_SAVE)
						{			
								// 此处需加入启动采集
								signal_process(ADC_buff, 12096, MAX_ADC_USE, &res);
								distance_temp = (uint16_t)(res.distance_mm );
								if(USE_UART){
									Modbus_SendDistance(distance_mm);
								}
									else Modbus_SendReadHoldingOneReg(distance_temp);
						}
						
						else if (start_addr == REG_DISTANCE_REAL)
						{		
								// 此处需加入启动采集
								uint8_t ret = signal_process(ADC_buff, 12096, MAX_ADC_USE, &res);

								if (ret == 0)
								{
										distance_mm = (uint16_t)(res.distance_mm );
										distance_temp = distance_mm;
										if(USE_UART){
											Modbus_SendDistance(distance_mm);
										}
											else 
												{
													Modbus_SendReadHoldingOneReg(distance_temp);
												};
								}
								else
								{		
										Modbus_SendException(func_code, 0x03);
								}
						}
						else
						{		
								Modbus_SendException(func_code, 0x02);
						}
						break;
			}
			// 06功能码
			case MB_FUNC_WRITE_SINGLE_REG: 
			{	
						start_addr = ((uint16_t)rx_buf[2] << 8) | rx_buf[3];
						quantity   = ((uint16_t)rx_buf[4] << 8) | rx_buf[5];
					if (start_addr == REG_CHANGE_BAUD)
					{
						uint32_t new_baudrate = 0U;

						switch (quantity)
						{
							case 0x0001: new_baudrate = 2400;   break;
							case 0x0002: new_baudrate = 4800;   break;
							case 0x0003: new_baudrate = 9600;   break;
							case 0x0004: new_baudrate = 14400;  break;
							case 0x0005: new_baudrate = 19200;  break;
							case 0x0006: new_baudrate = 38400;  break;
							case 0x0007: new_baudrate = 57600;  break;
							case 0x0008: new_baudrate = 76800;  break;
							case 0x0009: new_baudrate = 115200; break;

							default:
								break;
						}
							Modbus_SendWriteSingleRegAck(start_addr, quantity);
							USART1_SetBaudRate(new_baudrate);
					}
					
					if (start_addr == REG_TRANSMIT_ADC)
					{
						usart1_dma_send_u16(ADC_buff, 12096U);
					}	
			}
			//修改从机地址
			if (start_addr == REG_CHANGE_SLAVE)
			{
				uint8_t new_slave_addr;

				if ((quantity < 1U) || (quantity > 247U))
				{
					Modbus_SendException(func_code, 0x03);
				}
				else
				{
					new_slave_addr = (uint8_t)quantity;
					Modbus_SendWriteSingleRegAck(start_addr, quantity);
					slave_addr = new_slave_addr;
				}
			}
			default:
			break;
			
		}
		
	}
static uint16_t CRC16(uint8_t *buffer, uint16_t buffer_length) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < buffer_length; i++) {
        crc ^= buffer[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xA001;
            else         crc >>= 1;
        }
    }
    return crc;
}

static void Modbus_SendException(uint8_t func_code, uint8_t exception_code)
{
    uint8_t frame[5];
    uint16_t crc;

    frame[0] = slave_addr;
    frame[1] = func_code | 0x80;
    frame[2] = exception_code;

    crc = CRC16(frame, 3);

    frame[3] = (uint8_t)(crc & 0xFF);          // CRC低字节
    frame[4] = (uint8_t)((crc >> 8) & 0xFF);   // CRC高字节

	USART1_Transmit(frame, sizeof(frame));

}

static void Modbus_SendDistance(uint16_t distance_mm)
{
    static uint8_t frame[4];

    frame[0] = 0xFF;
		if(distance_mm <= 50)
		{
			frame[1] = 0x00;
			frame[2] = 0x32;
		}
		else if(distance_mm >= 2000)
		{
			frame[1] = 0x07;
			frame[2] = 0xD0;
		}
		else{
			frame[1] = (uint8_t)((distance_mm >> 8) & 0xFF);
			frame[2] = (uint8_t)(distance_mm & 0xFF);
		}
    frame[3] = (uint8_t)((frame[0] + frame[1] + frame[2]) & 0xFF);
    USART1_Transmit(frame, sizeof(frame));
}

// 标准Modbus RTU：0x03 读保持寄存器，返回1个寄存器
static void Modbus_SendReadHoldingOneReg(uint16_t value)
{
    uint8_t frame[7];
    uint16_t crc;

    frame[0] = slave_addr;              // 从机地址
    frame[1] = MB_FUNC_READ_HOLDING;    // 功能码 0x03
    frame[2] = 0x02;                    // 字节数：1个寄存器 = 2字节
    frame[3] = (uint8_t)(value >> 8);   // 数据高字节
    frame[4] = (uint8_t)(value & 0xFF); // 数据低字节

    crc = CRC16(frame, 5);

    frame[5] = (uint8_t)(crc & 0xFF);        // CRC低字节
    frame[6] = (uint8_t)((crc >> 8) & 0xFF); // CRC高字节

    USART1_Transmit(frame, sizeof(frame));
}

// 标准Modbus RTU：0x06 写单个寄存器成功返回
static void Modbus_SendWriteSingleRegAck(uint16_t reg_addr, uint16_t value)
{
    uint8_t frame[8];
    uint16_t crc;

    frame[0] = slave_addr;
    frame[1] = MB_FUNC_WRITE_SINGLE_REG;        // 功能码 0x06
    frame[2] = (uint8_t)(reg_addr >> 8);        // 寄存器地址高字节
    frame[3] = (uint8_t)(reg_addr & 0xFF);      // 寄存器地址低字节
    frame[4] = (uint8_t)(value >> 8);           // 写入值高字节
    frame[5] = (uint8_t)(value & 0xFF);         // 写入值低字节

    crc = CRC16(frame, 6);

    frame[6] = (uint8_t)(crc & 0xFF);           // CRC低字节
    frame[7] = (uint8_t)((crc >> 8) & 0xFF);    // CRC高字节

    USART1_Transmit(frame, sizeof(frame));
}
