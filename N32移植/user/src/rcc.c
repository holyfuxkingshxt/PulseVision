#include "rcc.h"
#include "n32g4fr_flash.h"
#include "system_n32g4fr.h"
#include "n32g4fr_rcc.h"

ErrorStatus  RCC_Configuration(void){
	RCC_DeInit();
	RCC_ConfigHse( RCC_HSE_ENABLE);
	RCC_WaitHseStable();

	if (RCC_WaitHseStable() != SUCCESS)
		return ERROR;
	
	FLASH_SetLatency(FLASH_LATENCY_4);
	
	RCC_ConfigHclk(RCC_SYSCLK_DIV1);
    RCC_ConfigPclk2(RCC_HCLK_DIV2);
    RCC_ConfigPclk1(RCC_HCLK_DIV4);
	
    /* 8MHz × 18 = 144MHz */
    RCC_ConfigPll(RCC_PLL_SRC_HSE_DIV1, RCC_PLL_MUL_18);
    RCC_EnablePll(ENABLE);

    while (RCC_GetFlagStatus(RCC_FLAG_PLLRD) == RESET)
    {
    }

    /* PLL作为系统时钟 */
    RCC_ConfigSysclk(RCC_SYSCLK_SRC_PLLCLK);

    while (RCC_GetSysclkSrc() != 0x08U)
    {
    }

    SystemCoreClockUpdate();

    return SUCCESS;
}

