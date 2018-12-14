/*  To enable circular buffer, you have to enable IDLE LINE DETECTION interrupt

__HAL_UART_ENABLE_ITUART_HandleTypeDef *huart, UART_IT_IDLE);   // enable idle line interrupt
__HAL_DMA_ENABLE_IT (DMA_HandleTypeDef *hdma, DMA_IT_TC);  // enable DMA Tx cplt interrupt

also enable RECEIVE DMA

HAL_UART_Receive_DMA (UART_HandleTypeDef *huart, DMA_RX_Buffer, 64);


IF you want to transmit the received data uncomment lines 91 and 101


PUT THE FOLLOWING IN THE MAIN.c

#define DMA_RX_BUFFER_SIZE          64
uint8_t DMA_RX_Buffer[DMA_RX_BUFFER_SIZE];

#define UART_BUFFER_SIZE            256
uint8_t UART_Buffer[UART_BUFFER_SIZE];


*/
//https://www.youtube.com/watch?v=tWryJb2L0cU
//https://www.controllerstech.com/receive-uart-data-using-dma-and-idle-line-detection/
#include "dma_circular.h"
#include "main.h"
//for timestamping of DMA ISR jump
#include "tim.h"

extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart2_rx;


extern uint8_t DMA_RX_Buffer[DMA_RX_BUFFER_SIZE];

extern uint32_t USART2CounterVal;

//MemPool For the NemaData  declatrated in main.cpp
//osPoolDef(NemaPool, NEMASTAMEDBUFFERSIZE, nemaDataStamped);
extern osPoolId NemaPool;
extern osMessageQId NemaMsgBuffer;

size_t Write;
size_t len, tocopy;
uint8_t* ptr;



void USART_IrqHandler(UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma)
{
	if ((huart->Instance->ISR & UART_FLAG_RXNE))
	{
		__HAL_UART_DISABLE_IT(&huart2,UART_IT_RXNE); //disable rx interupt and then enable idle line after first byte is recived
	    __HAL_UART_ENABLE_IT(&huart2,UART_IT_IDLE);   // enable idle line interrupt
	}
	if ((huart->Instance->ISR & UART_FLAG_IDLE))           /* if Idle flag is set */
	{
		volatile uint32_t tmp;                  /* Must be volatile to prevent optimizations */
        tmp = huart->Instance->ISR;                       /* Read status register */
        tmp = huart->Instance->TDR;                       /* Read data register */
		hdma->Instance->CR &= ~DMA_SxCR_EN;       /* Disabling DMA will force transfer complete interrupt if enabled */
	}
}


void DMA_UARTHandler(DMA_HandleTypeDef *hdma)
{
	uint32_t USART2CounterVal = __HAL_TIM_GetCounter(&htim2);
	typedef struct
	{
		__IO uint32_t ISR;   /*!< DMA interrupt status register */
		__IO uint32_t Reserved0;
		__IO uint32_t IFCR;  /*!< DMA interrupt flag clear register */
	} DMA_Base_Registers;

	DMA_Base_Registers *regs = (DMA_Base_Registers *)hdma->StreamBaseAddress;

	if(__HAL_DMA_GET_IT_SOURCE(hdma, DMA_IT_TC) != RESET)   // if the source is TC
	{
		/* Clear the transfer complete flag */
      regs->IFCR = DMA_FLAG_TCIF0_4 << hdma->StreamIndex;

	     /* Get the length of the data */
	  len = DMA_RX_BUFFER_SIZE - hdma->Instance->NDTR;
	  uint16_t Addroffset=0;
	  char * uEndOfSentenceIndex=strstr(DMA_RX_Buffer+Addroffset, "\r\n");
	  if(uEndOfSentenceIndex!=NULL){
	  uint32_t nemaLen=(uint32_t )uEndOfSentenceIndex-(uint32_t)DMA_RX_Buffer+Addroffset;
	  if (nemaLen<NEMASENTENCMAXLEN){
	  nemaDataStamped *mptr;
		// ATENTION!! if buffer is full the allocation function is blocking aprox 60µs
		mptr = osPoolCAlloc(NemaPool);
		if (mptr != NULL) {
			mptr->timestamp=USART2CounterVal;
			//TODO remove segfault
			memcpy((mptr->nemaSentence),&DMA_RX_Buffer[Addroffset],nemaLen);
			mptr->size=nemaLen;
			//put dater pointer into MSGQ
			osStatus result = osMessagePut(NemaMsgBuffer, (uint32_t)mptr,
			0);
		}
	  }
	  }
		/* Prepare DMA for next transfer */
        /* Important! DMA stream won't start if all flags are not cleared first */

        regs->IFCR = 0x3FU << hdma->StreamIndex; // clear all interrupts
		hdma->Instance->M0AR = (uint32_t)DMA_RX_Buffer;   /* Set memory address for DMA again */
        hdma->Instance->NDTR = DMA_RX_BUFFER_SIZE;    /* Set number of bytes to receive */
        hdma->Instance->CR |= DMA_SxCR_EN;            /* Start DMA transfer */
	}
}
