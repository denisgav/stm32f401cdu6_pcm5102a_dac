/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include "tusb_config.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ONBOARD_LED_Pin GPIO_PIN_13
#define ONBOARD_LED_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

#ifndef I2S_SPK_RATE_DEF
	#define I2S_SPK_RATE_DEF 48000
#endif //I2S_SPK_RATE_DEF

#ifndef m_new
    #define m_new(type, num) ((type *)(malloc(sizeof(type) * (num))))
#endif //m_new

typedef struct  {
    uint32_t left;
    uint32_t right;
} i2s_32b_audio_sample;

typedef struct  {
    uint16_t left;
    uint16_t right;
} i2s_16b_audio_sample;
//-------------------------
// I2s defines
//-------------------------
#define SAMPLE_BUFFER_SIZE  (CFG_TUD_AUDIO_FUNC_1_MAX_SAMPLE_RATE/1000) // MAX sample rate divided by 1000. Size of 1 ms sample

//-------------------------
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
