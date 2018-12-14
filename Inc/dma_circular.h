/*
 * dma_circular.h
 *
 *  Created on: 28.11.2018
 *      Author: seeger01
 */

#ifndef DMA_CIRCULAR_H_
#define DMA_CIRCULAR_H_
#include "stm32f7xx_hal.h"
#include "cmsis_os.h"
#ifdef __cplusplus

 extern "C" {

#endif

#define NEMASENTENCMAXLEN 84
 typedef struct {
     uint32_t timestamp;
     uint16_t size;
     uint8_t nemaSentence[NEMASENTENCMAXLEN];
  }nemaDataStamped;

void USART_IrqHandler (UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma);

void DMA_UARTHandler (DMA_HandleTypeDef *hdma);



#ifdef __cplusplus
}
#endif

#endif /* DMA_CIRCULAR_H_ */
