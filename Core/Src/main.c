/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "volume_ctrl.h"
#include "usb_speaker.h"
#include "ring_buf.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTOTYPES
//--------------------------------------------------------------------+
typedef struct _current_settings_t {
	// Audio controls
	// Current states
	int8_t mute[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1]; // +1 for master channel 0
	int16_t volume[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1]; // +1 for master channel 0
	int32_t volume_db[CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1]; // +1 for master channel 0

	uint32_t sample_rate;
	uint8_t resolution;
	uint32_t blink_interval_ms;

	uint16_t samples_in_i2s_frame_min;
	uint16_t samples_in_i2s_frame_max;

	bool mic_muted_by_user;
	bool mic_live_status;

	uint32_t cur_time_ms;

} current_settings_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s2;
DMA_HandleTypeDef hdma_spi2_tx;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */
current_settings_t current_settings;
ring_buf_t ring_buffer;
uint8_t *ring_buffer_storage;

// Buffer for speaker data
//i2s_32b_audio_sample spk_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_32b_audio_sample spk_32b_i2s_buffer[SAMPLE_BUFFER_SIZE];
i2s_16b_audio_sample spk_16b_i2s_buffer[SAMPLE_BUFFER_SIZE];
uint8_t spk_usb_read_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ];
uint8_t spk_i2s_buf[CFG_TUD_AUDIO_FUNC_1_EP_OUT_SW_BUF_SZ];

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S2_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
/* USER CODE BEGIN PFP */

void refresh_i2s_connections();
void led_blinking_task(void);
int32_t usb_to_i2s_32b_sample_convert(int32_t sample, int32_t volume_db);
int16_t usb_to_i2s_16b_sample_convert(int16_t sample, int32_t volume_db);

void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in);
void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in);
void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in);
void usb_speaker_current_resolution_handler(uint8_t current_resolution_in);
void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in);

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport,
		uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out,
		uint8_t cur_alt_setting);

int machine_i2s_write_stream(uint8_t *buf_in, size_t size);

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);

uint32_t feed_dma(uint8_t *dma_buffer_p, uint32_t sizeof_half_dma_buffer_in_bytes);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S2_Init();
  MX_USB_OTG_FS_PCD_Init();
  /* USER CODE BEGIN 2 */
	current_settings.sample_rate = I2S_SPK_RATE_DEF;
	current_settings.resolution = CFG_TUD_AUDIO_FUNC_1_RESOLUTION_RX;
	current_settings.blink_interval_ms = BLINK_NOT_MOUNTED;

	usb_speaker_set_mute_set_handler(usb_speaker_mute_handler);
	usb_speaker_set_volume_set_handler(usb_speaker_volume_handler);
	usb_speaker_set_current_sample_rate_set_handler(
			usb_speaker_current_sample_rate_handler);
	usb_speaker_set_current_resolution_set_handler(
			usb_speaker_current_resolution_handler);
	usb_speaker_set_current_status_set_handler(
			usb_speaker_current_status_set_handler);

	usb_speaker_set_tud_audio_rx_done_pre_read_set_handler(
			usb_speaker_tud_audio_rx_done_pre_read_handler);

	usb_speaker_init();
	refresh_i2s_connections();

	for (int i = 0; i < (CFG_TUD_AUDIO_FUNC_1_N_CHANNELS_RX + 1); i++) {
		current_settings.volume[i] = DEFAULT_VOLUME;
		current_settings.mute[i] = 0;
		current_settings.volume_db[i] = vol_to_db_convert(
				current_settings.mute[i], current_settings.volume[i]);
	}
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
		usb_speaker_task();
		led_blinking_task();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2S2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S2_Init(void)
{

  /* USER CODE BEGIN I2S2_Init 0 */

  /* USER CODE END I2S2_Init 0 */

  /* USER CODE BEGIN I2S2_Init 1 */

  /* USER CODE END I2S2_Init 1 */
  hi2s2.Instance = SPI2;
  hi2s2.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s2.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s2.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s2.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s2.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s2.Init.CPOL = I2S_CPOL_LOW;
  hi2s2.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S2_Init 2 */

  /* USER CODE END I2S2_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 4;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ONBOARD_LED_GPIO_Port, ONBOARD_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : ONBOARD_LED_Pin */
  GPIO_InitStruct.Pin = ONBOARD_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ONBOARD_LED_GPIO_Port, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void usb_speaker_mute_handler(int8_t bChannelNumber, int8_t mute_in) {
	current_settings.mute[bChannelNumber] = mute_in;
	current_settings.volume_db[bChannelNumber] = vol_to_db_convert(
			current_settings.mute[bChannelNumber],
			current_settings.volume[bChannelNumber]);
}

void usb_speaker_volume_handler(int8_t bChannelNumber, int16_t volume_in) {
	current_settings.volume[bChannelNumber] = volume_in;
	current_settings.volume_db[bChannelNumber] = vol_to_db_convert(
			current_settings.mute[bChannelNumber],
			current_settings.volume[bChannelNumber]);
}

void usb_speaker_current_sample_rate_handler(uint32_t current_sample_rate_in) {
	current_settings.sample_rate = current_sample_rate_in;
	refresh_i2s_connections();
}

void usb_speaker_current_resolution_handler(uint8_t current_resolution_in) {
	current_settings.resolution = current_resolution_in;
	refresh_i2s_connections();
}

void usb_speaker_current_status_set_handler(uint32_t blink_interval_ms_in) {
	current_settings.blink_interval_ms = blink_interval_ms_in;
}

void usb_speaker_tud_audio_rx_done_pre_read_handler(uint8_t rhport,
		uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out,
		uint8_t cur_alt_setting) {
	uint32_t volume_db_master = current_settings.volume_db[0];
	uint32_t volume_db_left = current_settings.volume_db[1];
	uint32_t volume_db_right = current_settings.volume_db[2];

	if (current_settings.blink_interval_ms == BLINK_STREAMING) {
		// Speaker data size received in the last frame
		uint16_t usb_spk_data_size = tud_audio_read(spk_usb_read_buf, n_bytes_received);
		uint16_t usb_sample_count = 0;

		if (current_settings.resolution == 16) {
			int16_t *in = (int16_t*) spk_usb_read_buf;
			usb_sample_count = usb_spk_data_size / 4; // 4 bytes per sample 2b left, 2b right

			//if (usb_sample_count >= current_settings.samples_in_i2s_frame_min) {
				for (int i = 0; i < usb_sample_count; i++) {
					int16_t left = in[i * 2 + 0];
					int16_t right = in[i * 2 + 1];

					left = usb_to_i2s_16b_sample_convert(left, volume_db_left);
					left = usb_to_i2s_16b_sample_convert(left,
							volume_db_master);
					right = usb_to_i2s_16b_sample_convert(right,
							volume_db_right);
					right = usb_to_i2s_16b_sample_convert(right,
							volume_db_master);
					spk_16b_i2s_buffer[i].left = left;
					spk_16b_i2s_buffer[i].right = right;
				}
				machine_i2s_write_stream(&spk_16b_i2s_buffer[0], usb_sample_count*4);
			//}
		} else if (current_settings.resolution == 24) {
			int32_t *in = (int32_t*) spk_usb_read_buf;
			usb_sample_count = usb_spk_data_size / 8; // 8 bytes per sample 4b left, 4b right

			//if (usb_sample_count >= current_settings.samples_in_i2s_frame_min) {
				for (int i = 0; i < usb_sample_count; i++) {
					int32_t left = in[i * 2 + 0];
					int32_t right = in[i * 2 + 1];

					left = usb_to_i2s_32b_sample_convert(left, volume_db_left);
					left = usb_to_i2s_32b_sample_convert(left,
							volume_db_master);
					right = usb_to_i2s_32b_sample_convert(right,
							volume_db_right);
					right = usb_to_i2s_32b_sample_convert(right,
							volume_db_master);
					spk_32b_i2s_buffer[i].left = left;
					spk_32b_i2s_buffer[i].right = right;
				}
				machine_i2s_write_stream(&spk_32b_i2s_buffer[0], usb_sample_count*8);
			//}
		}
	}
}

int32_t usb_to_i2s_32b_sample_convert(int32_t sample, int32_t volume_db) {
	int64_t sample_tmp = (int64_t) sample * (int64_t) volume_db;
	sample_tmp = sample_tmp >> 15;
	return (int32_t) sample_tmp;
	//return (int32_t)sample;
}

int16_t usb_to_i2s_16b_sample_convert(int16_t sample, int32_t volume_db) {
	int32_t sample_tmp = (int32_t) sample * (int32_t) volume_db;
	sample_tmp = sample_tmp >> 15;
	return (int16_t) sample_tmp;
	//return (int16_t)sample;
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void refresh_i2s_connections() {
	HAL_I2S_StateTypeDef dma_state =  HAL_I2S_GetState(&hi2s2);
	if(dma_state != HAL_I2S_STATE_READY){
		HAL_StatusTypeDef stop_status = HAL_I2S_DMAStop(&hi2s2);
		if (stop_status != HAL_OK) {
			Error_Handler();
		}
	}

	if(ring_buffer_storage != NULL){
		free(ring_buffer_storage);
	}

	current_settings.samples_in_i2s_frame_min = (current_settings.sample_rate)
			/ 1000;
	current_settings.samples_in_i2s_frame_max = (current_settings.sample_rate
			+ 999) / 1000;
	current_settings.mic_muted_by_user = false;
	current_settings.cur_time_ms = 0;

	uint8_t appbuf_sample_size_in_bytes = (current_settings.resolution == 24) ? (4 * 2) : (2 * 2);

	ring_buffer_storage = m_new(uint8_t, current_settings.samples_in_i2s_frame_max*appbuf_sample_size_in_bytes*2);
	ringbuf_init(&ring_buffer, ring_buffer_storage, current_settings.samples_in_i2s_frame_max*appbuf_sample_size_in_bytes*2);

	// Run transmit DMA
	memset(spk_i2s_buf, 0x0, sizeof(spk_i2s_buf));
	HAL_StatusTypeDef tx_status = HAL_I2S_Transmit_DMA(&hi2s2, spk_i2s_buf,
			current_settings.samples_in_i2s_frame_max*2);
	if (tx_status != HAL_OK) {
		Error_Handler();
	}

//  speaker_i2s0 = create_machine_i2s(0, I2S_SPK_SCK, I2S_SPK_WS, I2S_SPK_SD, TX,
//    ((current_settings.resolution == 16) ? 16 : 32), STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, current_settings.sample_rate);
}
void led_blinking_task(void) {
	static uint32_t start_ms = 0;
	static bool led_state = false;

	current_settings.cur_time_ms = board_millis();

	// Blink every interval ms
	if (current_settings.cur_time_ms - start_ms
			< current_settings.blink_interval_ms)
		return;
	start_ms += current_settings.blink_interval_ms;

	board_led_write(led_state);
	led_state = 1 - led_state;
}

int machine_i2s_write_stream(uint8_t *buf_in, size_t size){
	uint8_t appbuf_sample_size_in_bytes = (current_settings.resolution == 24) ? (4 * 2) : (2 * 2);
	if (size % appbuf_sample_size_in_bytes != 0) {
		return -2;
	}

	if (size == 0) {
		return 0;
	}

	// copy audio samples from the app buffer to the ring buffer
	// loop, reading samples until the app buffer is emptied
	// for uasyncio mode, the loop will make an early exit if the ring buffer becomes full

	uint32_t a_index = 0;

	while (a_index < size) {
		// copy a byte to the ringbuf when space becomes available
		while (ringbuf_push(&ring_buffer, buf_in[a_index]) == false) {
			;
		}
		a_index++;
	}

	return a_index;
}

void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s) {
	uint32_t transfer_size_in_bytes = (current_settings.resolution == 24) ? (4 * 2) : (2 * 2);
	uint32_t sizeof_half_dma_buffer_in_bytes = current_settings.samples_in_i2s_frame_max*transfer_size_in_bytes/2;
	feed_dma(&spk_i2s_buf[0], sizeof_half_dma_buffer_in_bytes);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s) {
	uint32_t transfer_size_in_bytes = (current_settings.resolution == 24) ? (4 * 2) : (2 * 2);
	uint32_t sizeof_half_dma_buffer_in_bytes = current_settings.samples_in_i2s_frame_max*transfer_size_in_bytes/2;
	feed_dma(&spk_i2s_buf[sizeof_half_dma_buffer_in_bytes], sizeof_half_dma_buffer_in_bytes);
}

uint32_t feed_dma(uint8_t *dma_buffer_p, uint32_t sizeof_half_dma_buffer_in_bytes){
	// when data exists, copy samples from ring buffer
	uint32_t available_data_bytes = ringbuf_available_data(&ring_buffer);
	if(available_data_bytes > sizeof_half_dma_buffer_in_bytes)
	{
		available_data_bytes = sizeof_half_dma_buffer_in_bytes;
	}

	if (available_data_bytes >= sizeof_half_dma_buffer_in_bytes) {
		for (uint32_t i = 0; i < available_data_bytes; i++) {
			ringbuf_pop(&ring_buffer, &dma_buffer_p[i]);
		}
		return available_data_bytes;
	} else {
		// underflow.  clear buffer to transmit "silence" on the I2S bus
		memset(dma_buffer_p, 0, sizeof_half_dma_buffer_in_bytes);
		return sizeof_half_dma_buffer_in_bytes;
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
