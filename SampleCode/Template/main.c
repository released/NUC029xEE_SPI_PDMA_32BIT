/*_____ I N C L U D E S ____________________________________________________*/
#include <stdio.h>
#include <string.h>
#include "NuMicro.h"

#include "misc_config.h"

/*_____ D E C L A R A T I O N S ____________________________________________*/
#define PLL_CLOCK   							72000000

struct flag_32bit flag_PROJ_CTL;
#define FLAG_PROJ_TIMER_PERIOD_1000MS                 	(flag_PROJ_CTL.bit0)
#define FLAG_PROJ_SPI_TRANSMIT_END                      (flag_PROJ_CTL.bit1)
#define FLAG_PROJ_SPI_TRANSMIT_PDMA                 	(flag_PROJ_CTL.bit2)
#define FLAG_PROJ_SPI_TRANSMIT_NONPDMA                  (flag_PROJ_CTL.bit3)
#define FLAG_PROJ_REVERSE4                              (flag_PROJ_CTL.bit4)
#define FLAG_PROJ_REVERSE5                              (flag_PROJ_CTL.bit5)
#define FLAG_PROJ_REVERSE6                              (flag_PROJ_CTL.bit6)
#define FLAG_PROJ_REVERSE7                              (flag_PROJ_CTL.bit7)

/*_____ D E F I N I T I O N S ______________________________________________*/

volatile unsigned int counter_systick = 0;
volatile uint32_t counter_tick = 0;

// #define ENABLE_AUTO_SS

#define SPI_FREQ 								(200000ul)

#define SPI0_MASTER_TX_DMA_CH   				(0)
#define SPI0_MASTER_OPENED_CH_TX   				(1 << SPI0_MASTER_TX_DMA_CH)

#if defined (ENABLE_AUTO_SS)
#define SPI0_SET_CS_LOW							((void) NULL)//(SPI_SET_SS_LOW(SPI0))//(PC0 = 0)
#define SPI0_SET_CS_HIGH						((void) NULL)//(SPI_SET_SS_HIGH(SPI0))//(PC0 = 1)
#else
#define SPI0_SET_CS_LOW							(PC0 = 0)
#define SPI0_SET_CS_HIGH						(PC0 = 1)
#endif

#define SPI_DATA_LEN 							(16)
uint32_t Spi0TxBuffer[SPI_DATA_LEN] = {0};

/*_____ M A C R O S ________________________________________________________*/

/*_____ F U N C T I O N S __________________________________________________*/

unsigned int get_systick(void)
{
	return (counter_systick);
}

void set_systick(unsigned int t)
{
	counter_systick = t;
}

void systick_counter(void)
{
	counter_systick++;
}

void SysTick_Handler(void)
{

    systick_counter();

    if (get_systick() >= 0xFFFFFFFF)
    {
        set_systick(0);      
    }

    // if ((get_systick() % 1000) == 0)
    // {
       
    // }

    #if defined (ENABLE_TICK_EVENT)
    TickCheckTickEvent();
    #endif    
}

void SysTick_delay(unsigned int delay)
{  
    
    unsigned int tickstart = get_systick(); 
    unsigned int wait = delay; 

    while((get_systick() - tickstart) < wait) 
    { 
    } 

}

void SysTick_enable(unsigned int ticks_per_second)
{
    set_systick(0);
    if (SysTick_Config(SystemCoreClock / ticks_per_second))
    {
        /* Setup SysTick Timer for 1 second interrupts  */
        printf("Set system tick error!!\n");
        while (1);
    }

    #if defined (ENABLE_TICK_EVENT)
    TickInitTickEvent();
    #endif
}

uint32_t get_tick(void)
{
	return (counter_tick);
}

void set_tick(uint32_t t)
{
	counter_tick = t;
}

void tick_counter(void)
{
	counter_tick++;
    if (get_tick() >= 60000)
    {
        set_tick(0);
    }
}

void delay_ms(uint16_t ms)
{
	#if 1
    uint32_t tickstart = get_tick();
    uint32_t wait = ms;
	uint32_t tmp = 0;
	
    while (1)
    {
		if (get_tick() > tickstart)	// tickstart = 59000 , tick_counter = 60000
		{
			tmp = get_tick() - tickstart;
		}
		else // tickstart = 59000 , tick_counter = 2048
		{
			tmp = 60000 -  tickstart + get_tick();
		}		
		
		if (tmp > wait)
			break;
    }
	
	#else
	TIMER_Delay(TIMER0, 1000*ms);
	#endif
}


void SPI_ClrBuffer(uint8_t idx)
{
    switch(idx)
    {
        case 0:
            reset_buffer(Spi0TxBuffer,0x00,SPI_DATA_LEN);        
            break;
    }
}

void SPI_SET_CS_LOW(SPI_T *spi)
{
    if (spi == SPI0)
    {
        SPI0_SET_CS_LOW;
    }	

}

void SPI_SET_CS_HIGH(SPI_T *spi)
{
    if (spi == SPI0)
    {
        SPI0_SET_CS_HIGH;
    }	

}

void SPI_transmit_finish(SPI_T *spi)
{
    // while(!FLAG_PROJ_SPI_TRANSMIT_END);	
    while (SPI_IS_BUSY(spi));
    SPI_SET_CS_HIGH(spi);
}

void SPI_transmit_TxStart(SPI_T *spi , uint32_t* SpiBuffer , uint32_t len)
{ 
    uint32_t i = 0;

    SPI_SET_CS_LOW(spi);

    //TX
    for (i = 0 ; i < len ; i++)
    {
        SPI_WRITE_TX(spi, SpiBuffer[i]);
        SPI_TRIGGER(spi);	
        while (SPI_IS_BUSY(spi));
    }   
     
    SPI_SET_CS_HIGH(spi);

    printf("%s done\n",__FUNCTION__);    
}


void SPI_PDMA_transmit_TxStart(SPI_T *spi , uint32_t* SpiBuffer , uint32_t len)
{
	FLAG_PROJ_SPI_TRANSMIT_END = DISABLE;

    if (spi == SPI0)
    {
        SPI_SET_CS_LOW(spi);

        //TX
        PDMA_SetTransferCnt(SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_32, len);
        PDMA_SetTransferAddr(SPI0_MASTER_TX_DMA_CH, (uint32_t)SpiBuffer, PDMA_SAR_INC, (uint32_t)&spi->TX, PDMA_DAR_FIX);		
        /* Set request source; set basic mode. */
        PDMA_SetTransferMode(SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);

        PDMA_Trigger(SPI0_MASTER_TX_DMA_CH);
        
        SPI_TRIGGER_TX_PDMA(spi);

        PDMA_EnableInt(SPI0_MASTER_TX_DMA_CH, PDMA_IER_BLKD_IE_Msk);	
    }	
    
    SPI_transmit_finish(spi);	

    if (spi == SPI0)
    {
        PDMA_DisableInt(SPI0_MASTER_TX_DMA_CH, PDMA_IER_BLKD_IE_Msk);
    }	

    printf("%s done\n",__FUNCTION__);    
}

void PDMA_IRQHandler(void)
{
    #if 1
    uint32_t status = PDMA_GET_INT_STATUS();

    if (status & SPI0_MASTER_OPENED_CH_TX )
    {
        /* Get PDMA Block transfer down interrupt status */
        if(PDMA_GET_CH_INT_STS(SPI0_MASTER_TX_DMA_CH) & PDMA_ISR_BLKD_IF_Msk)
        {
            /* Clear PDMA Block transfer down interrupt flag */   
            PDMA_CLR_CH_INT_FLAG(SPI0_MASTER_TX_DMA_CH, PDMA_ISR_BLKD_IF_Msk);   
            
            /* Handle PDMA block transfer done interrupt event */
            //insert process
            FLAG_PROJ_SPI_TRANSMIT_END = ENABLE;      
            // printf("%s done\n",__FUNCTION__);  
        }
    }
    else
    {
        printf("unknown interrupt !!\n");
    }

    #else
    uint32_t status = PDMA_GET_INT_STATUS();

    if(status & 0x1) {  /* CH0 */
        if(PDMA_GET_CH_INT_STS(0) & 0x2)
        {
  			//insert process
			FLAG_PROJ_SPI_TRANSMIT_END = ENABLE;          
        }
        PDMA_CLR_CH_INT_FLAG(0, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x2) {  /* CH1 */
        if(PDMA_GET_CH_INT_STS(1) & 0x2)
        {
			//insert process
			FLAG_PROJ_SPI_TRANSMIT_END = ENABLE;            
        }
        PDMA_CLR_CH_INT_FLAG(1, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x4) {  /* CH2 */
        if(PDMA_GET_CH_INT_STS(2) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(2, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x8) {  /* CH3 */
        if(PDMA_GET_CH_INT_STS(3) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(3, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x10) {  /* CH4 */
        if(PDMA_GET_CH_INT_STS(4) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(4, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x20) {  /* CH5 */
        if(PDMA_GET_CH_INT_STS(5) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(5, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x40) {  /* CH6 */
        if(PDMA_GET_CH_INT_STS(6) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(6, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x80) {  /* CH7 */
        if(PDMA_GET_CH_INT_STS(7) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(7, PDMA_ISR_BLKD_IF_Msk);
    } else if(status & 0x100) {  /* CH8 */
        if(PDMA_GET_CH_INT_STS(8) & 0x2)
        {
            
        }
        PDMA_CLR_CH_INT_FLAG(8, PDMA_ISR_BLKD_IF_Msk);
    } else
        printf("unknown interrupt !!\n");
    #endif
}

void SPI_PDMA_Init(SPI_T *spi)
{
    PDMA_T *pdma;

	FLAG_PROJ_SPI_TRANSMIT_END = DISABLE;

    // SPI_SET_CS_LOW(spi);

    if (spi == SPI0)
    {
        /* Open PDMA Channel */
        PDMA_Open(SPI0_MASTER_OPENED_CH_TX );

        //TX
        PDMA_SetTransferCnt(SPI0_MASTER_TX_DMA_CH, PDMA_WIDTH_32, SPI_DATA_LEN);
        /* Set source/destination address and attributes */
        PDMA_SetTransferAddr(SPI0_MASTER_TX_DMA_CH, (uint32_t)Spi0TxBuffer, PDMA_SAR_INC, (uint32_t)&spi->TX, PDMA_DAR_FIX);
        /* Set request source; set basic mode. */
        PDMA_SetTransferMode(SPI0_MASTER_TX_DMA_CH, PDMA_SPI0_TX, FALSE, 0);

        /* Set Memory-to-Peripheral mode */
        pdma = (PDMA_T *)((uint32_t) PDMA0_BASE + (0x100 * SPI0_MASTER_TX_DMA_CH));
        pdma->CSR = (pdma->CSR & (~PDMA_CSR_MODE_SEL_Msk)) | (0x2<<PDMA_CSR_MODE_SEL_Pos);

        // SPI_TRIGGER_TX_PDMA(spi);	
        
        // PDMA_EnableInt(SPI0_MASTER_TX_DMA_CH, PDMA_IER_BLKD_IE_Msk);

        NVIC_EnableIRQ(PDMA_IRQn);

        // PDMA_Trigger(SPI0_MASTER_TX_DMA_CH);

    }	

	// SPI_transmit_finish(spi);	
}


void SPI_Init(SPI_T *spi)
{
    SPI_ClrBuffer(SPI0_MASTER_TX_DMA_CH);

    SPI_Open(spi, SPI_MASTER, SPI_MODE_0, 32, SPI_FREQ);    /*CPOL:LOW,CPHA:LOW*/
    // SPI_Open(spi, SPI_MASTER, SPI_MODE_1, 32, SPI_FREQ);    /*CPOL:LOW,CPHA:HIGH*/
    // SPI_Open(spi, SPI_MASTER, SPI_MODE_2, 32, SPI_FREQ);    /*CPOL:HIGH,CPHA:LOW*/
    // SPI_Open(spi, SPI_MASTER, SPI_MODE_3, 32, SPI_FREQ);    /*CPOL:HIGH,CPHA:HIGH*/

    SPI_EnableFIFO(spi, 4, 4);

    #if defined (ENABLE_AUTO_SS)
    SPI_EnableAutoSS(spi, SPI_SS, SPI_SS_ACTIVE_LOW);
    #else
    SYS_UnlockReg();

    SYS->GPC_MFP &= ~(SYS_GPC_MFP_PC0_Msk);
    SYS->GPC_MFP |= SYS_GPC_MFP_PC0_GPIO;
    SYS->ALT_MFP &= ~(SYS_ALT_MFP_PC0_Msk);
    SYS->ALT_MFP |= SYS_ALT_MFP_PC0_GPIO;

    // USE SS : PC0 as manual control , 
    GPIO_SetMode(PC, BIT0, GPIO_PMD_OUTPUT);
    SYS_LockReg();

    SPI_DisableAutoSS(spi);

    #endif

    SPI_SET_CS_HIGH(spi);
    SPI_PDMA_Init(spi);

}


void SPI0_process(void)
{
	uint16_t i = 0;	
	static uint8_t data_cnt = 1;
    uint8_t target_len = 0;
    uint32_t default_val = 0x5A5AA500;

	if (FLAG_PROJ_SPI_TRANSMIT_PDMA)
	{
		FLAG_PROJ_SPI_TRANSMIT_PDMA = DISABLE;
        SPI_ClrBuffer(SPI0_MASTER_TX_DMA_CH);       

		for (i = 0 ; i < SPI_DATA_LEN ; i++ )
		{
			Spi0TxBuffer[i] = default_val|0x80 + i ;
		}

		Spi0TxBuffer[SPI_DATA_LEN-2] = default_val|0x90 + data_cnt;	
		Spi0TxBuffer[SPI_DATA_LEN-1] = default_val|0x92 + data_cnt;
		
		SPI_PDMA_transmit_TxStart(SPI0 , Spi0TxBuffer , SPI_DATA_LEN);

		data_cnt = (data_cnt > 0x9) ? (1) : (data_cnt+1); 
	
	}

	if (FLAG_PROJ_SPI_TRANSMIT_NONPDMA)
	{
		FLAG_PROJ_SPI_TRANSMIT_NONPDMA = DISABLE;
        SPI_ClrBuffer(SPI0_MASTER_TX_DMA_CH);

        target_len = SPI_DATA_LEN;
  
		for (i = 0 ; i < target_len ; i++ )
		{
			Spi0TxBuffer[i] = default_val|0x40 + i ;
		}

		Spi0TxBuffer[target_len-2] = default_val|0xAA + data_cnt;	
		Spi0TxBuffer[target_len-1] = default_val|0xCC + data_cnt;
		
		SPI_transmit_TxStart(SPI0 , Spi0TxBuffer , target_len);

		data_cnt = (data_cnt > 0x9) ? (1) : (data_cnt+1); 
    }
}

//
// check_reset_source
//
uint8_t check_reset_source(void)
{
    uint32_t src = SYS_GetResetSrc();

    SYS->RSTSRC |= 0x0FF;
    printf("Reset Source <0x%08X>\r\n", src);

    #if 1   //DEBUG , list reset source
    if (src & BIT0)
    {
        printf("0)Power-On Reset Flag\r\n");       
    }
    if (src & BIT1)
    {
        printf("1)Reset Pin Reset Flag\r\n");       
    }
    if (src & BIT2)
    {
        printf("2)Watchdog Timer Reset Flag\r\n");       
    }
    if (src & BIT3)
    {
        printf("3)Low Voltage Reset Flag\r\n");       
    }
    if (src & BIT4)
    {
        printf("4)Brown-Out Detector Reset Flag\r\n");       
    }
    if (src & BIT5)
    {
        printf("5)SYS Reset Flag\r\n");       
    }
    if (src & BIT6)
    {
        printf("6)Reserved.\r\n");       
    }
    if (src & BIT7)
    {
        printf("7)CPU Reset Flag \r\n");       
    }
    #endif
    
    if (src & SYS_RSTSRC_RSTS_POR_Msk) {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_POR_Msk);
        
        printf("power on from POR\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_RESET_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_RESET_Msk);
        
        printf("power on from nRESET pin\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSRC_RSTS_WDT_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_WDT_Msk);
        
        printf("power on from WDT Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_LVR_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_LVR_Msk);
        
        printf("power on from LVR Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_BOD_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_BOD_Msk);
        
        printf("power on from BOD Reset\r\n");
        return FALSE;
    }    
    else if (src & SYS_RSTSRC_RSTS_SYS_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_SYS_Msk);
        
        printf("power on from System Reset\r\n");
        return FALSE;
    } 
    else if (src & SYS_RSTSRC_RSTS_CPU_Msk)
    {
        SYS_ClearResetSrc(SYS_RSTSRC_RSTS_CPU_Msk);

        printf("power on from CPU reset\r\n");
        return FALSE;         
    } 
    
    printf("power on from unhandle reset source\r\n");
    return FALSE;
}

void TMR1_IRQHandler(void)
{
	
    if(TIMER_GetIntFlag(TIMER1) == 1)
    {
        TIMER_ClearIntFlag(TIMER1);
		tick_counter();

		if ((get_tick() % 1000) == 0)
		{
            FLAG_PROJ_TIMER_PERIOD_1000MS = 1;//set_flag(flag_timer_period_1000ms ,ENABLE);
		}

		if ((get_tick() % 50) == 0)
		{

		}	
    }
}

void TIMER1_Init(void)
{
    TIMER_Open(TIMER1, TIMER_PERIODIC_MODE, 1000);
    TIMER_EnableInt(TIMER1);
    NVIC_EnableIRQ(TMR1_IRQn);	
    TIMER_Start(TIMER1);
}

void loop(void)
{
	// static uint32_t LOG1 = 0;
	// static uint32_t LOG2 = 0;

    if ((get_systick() % 1000) == 0)
    {
        // printf("%s(systick) : %4d\r\n",__FUNCTION__,LOG2++);    
    }

    if (FLAG_PROJ_TIMER_PERIOD_1000MS)//(is_flag_set(flag_timer_period_1000ms))
    {
        FLAG_PROJ_TIMER_PERIOD_1000MS = 0;//set_flag(flag_timer_period_1000ms ,DISABLE);

        // printf("%s(timer) : %4d\r\n",__FUNCTION__,LOG1++);
        PB4 ^= 1;        
    }

    SPI0_process();
}

void UARTx_Process(void)
{
	uint8_t res = 0;
	res = UART_READ(UART0);

	if (res > 0x7F)
	{
		printf("invalid command\r\n");
	}
	else
	{
		printf("press : %c\r\n" , res);
		switch(res)
		{
			case '1':
                FLAG_PROJ_SPI_TRANSMIT_PDMA = ENABLE;
				break;
			case '2':
                FLAG_PROJ_SPI_TRANSMIT_NONPDMA = ENABLE;
				break;

			case 'X':
			case 'x':
			case 'Z':
			case 'z':
                SYS_UnlockReg();
				// NVIC_SystemReset();	// Reset I/O and peripherals , only check BS(FMC_ISPCTL[1])
                // SYS_ResetCPU();     // Not reset I/O and peripherals
                SYS_ResetChip();    // Reset I/O and peripherals ,  BS(FMC_ISPCTL[1]) reload from CONFIG setting (CBS)	
				break;
		}
	}
}

void UART02_IRQHandler(void)
{

    if(UART_GET_INT_FLAG(UART0, UART_ISR_RDA_INT_Msk | UART_ISR_TOUT_IF_Msk))     /* UART receive data available flag */
    {
        while(UART_GET_RX_EMPTY(UART0) == 0)
        {
            UARTx_Process();
        }
    }

}

void UART0_Init(void)
{
    SYS_ResetModule(UART0_RST);

    /* Configure UART0 and set UART0 baud rate */
    UART_Open(UART0, 115200);
    UART_EnableInt(UART0, UART_IER_RDA_IEN_Msk | UART_IER_TOUT_IEN_Msk);
    NVIC_EnableIRQ(UART02_IRQn);
	
	#if (_debug_log_UART_ == 1)	//debug
	printf("\r\nCLK_GetCPUFreq : %8d\r\n",CLK_GetCPUFreq());
	printf("CLK_GetHCLKFreq : %8d\r\n",CLK_GetHCLKFreq());
	printf("CLK_GetHXTFreq : %8d\r\n",CLK_GetHXTFreq());
	printf("CLK_GetLXTFreq : %8d\r\n",CLK_GetLXTFreq());	
	printf("CLK_GetPCLKFreq : %8d\r\n",CLK_GetPCLKFreq());	
	#endif	

    #if 0
    printf("FLAG_PROJ_TIMER_PERIOD_1000MS : 0x%2X\r\n",FLAG_PROJ_TIMER_PERIOD_1000MS);
    printf("FLAG_PROJ_REVERSE1 : 0x%2X\r\n",FLAG_PROJ_REVERSE1);
    printf("FLAG_PROJ_REVERSE2 : 0x%2X\r\n",FLAG_PROJ_REVERSE2);
    printf("FLAG_PROJ_REVERSE3 : 0x%2X\r\n",FLAG_PROJ_REVERSE3);
    printf("FLAG_PROJ_REVERSE4 : 0x%2X\r\n",FLAG_PROJ_REVERSE4);
    printf("FLAG_PROJ_REVERSE5 : 0x%2X\r\n",FLAG_PROJ_REVERSE5);
    printf("FLAG_PROJ_REVERSE6 : 0x%2X\r\n",FLAG_PROJ_REVERSE6);
    printf("FLAG_PROJ_REVERSE7 : 0x%2X\r\n",FLAG_PROJ_REVERSE7);
    #endif

}

void GPIO_Init (void)
{
    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB4_Msk);
    SYS->GPB_MFP |= (SYS_GPB_MFP_PB4_GPIO);
	
    GPIO_SetMode(PB, BIT4, GPIO_PMD_OUTPUT);

}


void SYS_Init(void)
{
    /* Unlock protected registers */
    SYS_UnlockReg();

    CLK_EnableXtalRC(CLK_PWRCON_OSC22M_EN_Msk);
    CLK_WaitClockReady(CLK_CLKSTATUS_OSC22M_STB_Msk);

    /* Switch HCLK clock source to Internal RC and HCLK source divide 1 */
    CLK_SetHCLK(CLK_CLKSEL0_HCLK_S_HIRC, CLK_CLKDIV_HCLK(1));

    /* Enable external XTAL 12MHz clock */
//    CLK_EnableXtalRC(CLK_PWRCON_XTL12M_EN_Msk);

    /* Waiting for external XTAL clock ready */
//    CLK_WaitClockReady(CLK_CLKSTATUS_XTL12M_STB_Msk);

    /* Set core clock as PLL_CLOCK from PLL */
    CLK_SetCoreClock(PLL_CLOCK);

    CLK_EnableModuleClock(UART0_MODULE);
    CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART_S_HIRC, CLK_CLKDIV_UART(1));
	
    CLK_EnableModuleClock(TMR1_MODULE);
  	CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1_S_HIRC, 0);

    SYS->GPB_MFP &= ~(SYS_GPB_MFP_PB0_Msk | SYS_GPB_MFP_PB1_Msk);

    SYS->GPB_MFP |= (SYS_GPB_MFP_PB0_UART0_RXD | SYS_GPB_MFP_PB1_UART0_TXD);

    CLK_EnableModuleClock(PDMA_MODULE);

    CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL1_SPI0_S_HCLK, MODULE_NoMsk);
    CLK_EnableModuleClock(SPI0_MODULE);

    SYS->GPC_MFP &= ~(SYS_GPC_MFP_PC0_Msk | SYS_GPC_MFP_PC1_Msk | SYS_GPC_MFP_PC2_Msk | SYS_GPC_MFP_PC3_Msk);
    SYS->GPC_MFP |= SYS_GPC_MFP_PC0_SPI0_SS0 | SYS_GPC_MFP_PC1_SPI0_CLK | SYS_GPC_MFP_PC2_SPI0_MISO0 | SYS_GPC_MFP_PC3_SPI0_MOSI0;
    SYS->ALT_MFP &= ~(SYS_ALT_MFP_PC0_Msk | SYS_ALT_MFP_PC1_Msk | SYS_ALT_MFP_PC2_Msk | SYS_ALT_MFP_PC3_Msk);
    SYS->ALT_MFP |= SYS_ALT_MFP_PC0_SPI0_SS0 | SYS_ALT_MFP_PC1_SPI0_CLK | SYS_ALT_MFP_PC2_SPI0_MISO0 | SYS_ALT_MFP_PC3_SPI0_MOSI0;

   /* Update System Core Clock */
    SystemCoreClockUpdate();

    /* Lock protected registers */
    SYS_LockReg();
}

int main()
{
    SYS_Init();

	GPIO_Init();
	UART0_Init();
	TIMER1_Init();
    check_reset_source();

    SysTick_enable(1000);
    #if defined (ENABLE_TICK_EVENT)
    TickSetTickEvent(1000, TickCallback_processA);  // 1000 ms
    TickSetTickEvent(5000, TickCallback_processB);  // 5000 ms
    #endif

    SPI_Init(SPI0);

    /* Got no where to go, just loop forever */
    while(1)
    {
        loop();

    }
}

/*** (C) COPYRIGHT 2017 Nuvoton Technology Corp. ***/
