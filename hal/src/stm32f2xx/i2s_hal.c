/* Created and copyrighted by Zachary J. Fields. Offered as open source under the MIT License (MIT). */

#include "i2s_hal.h"

#include <misc.h>
#include <stm32f2xx.h>
#include <stm32f2xx_dma.h>
#include <stm32f2xx_gpio.h>
#include <stm32f2xx_rcc.h>
#include <stm32f2xx_spi.h>

//TODO: Clarify how to yield the processor
//#include "concurrent_hal.h"
#include "gpio_hal.h"
#include "hal_irq_flag.h"
#include "pinmap_impl.h"

#if PLATFORM_ID == 10 // Electron
  #define TOTAL_I2S   2
#else
  #define TOTAL_I2S   1
#endif

static const size_t I2S_TX_TIMEOUT_MS = 3000;

typedef struct i2s_pin_set_t {
    uint8_t I2S_SCK_Pin;
    uint8_t I2S_SD_Pin;
    uint8_t I2S_WS_Pin;
} i2s_pin_set_t;

typedef struct stm32_i2s_req_t {
    SPI_TypeDef * I2S_Peripheral;

    uint32_t I2S_RCC_APB1Periph;
    uint32_t DMA_RCC_AHBRegister;
    uint32_t I2S_DMA_Channel;

    DMA_Stream_TypeDef * I2S_EXT_TX_DMA_Stream;

    uint8_t I2S_EXT_TX_DMA_Stream_IRQn;
    uint32_t I2S_EXT_TX_DMA_Stream_TC_Event;

    i2s_pin_set_t i2s_pins[TOTAL_I2S];

    uint8_t I2S_AF_Mapping;
} stm32_i2s_req_t;

/*
 * I2S mapping
 */
static const stm32_i2s_req_t i2s_req =
{
  SPI3,
  RCC_APB1Periph_SPI3,
  RCC_AHB1Periph_DMA2,
  DMA_Channel_2,
  DMA1_Stream5,
  DMA1_Stream5_IRQn,
  DMA_IT_TCIF5,
  {
    { D4, D2, D5 }
#if PLATFORM_ID == 10 // Electron
    ,{ C3, C1, A6 }
#endif
  },
  GPIO_AF_SPI3
};

typedef struct i2s_state_t {
    HAL_I2S_Interface active_interface;

    I2S_InitTypeDef i2s_config;
    DMA_InitTypeDef dma_config;
    NVIC_InitTypeDef nvic_config;

    bool initialized;
    bool ready;

    hal_i2s_callback_t tx_callback;
    void * tx_context;
} i2s_state_t;

static i2s_state_t i2s;

/**
 * @brief  This function handles DMA1 Stream 5 interrupt request.
 * @param  None
 * @retval None
 */
void DMA1_Stream5_IRQHandler (void)
{
    if (DMA_GetITStatus(i2s_req.I2S_EXT_TX_DMA_Stream, i2s_req.I2S_EXT_TX_DMA_Stream_TC_Event) == SET)
    {
        // Deactivate I2S and DMA
        DMA_ClearITPendingBit(i2s_req.I2S_EXT_TX_DMA_Stream, i2s_req.I2S_EXT_TX_DMA_Stream_TC_Event);
        DMA_Cmd(i2s_req.I2S_EXT_TX_DMA_Stream, DISABLE);
        I2S_Cmd(i2s_req.I2S_Peripheral, DISABLE);
        SPI_I2S_DMACmd(i2s_req.I2S_Peripheral, SPI_I2S_DMAReq_Tx, DISABLE);
        DMA_DeInit(i2s_req.I2S_EXT_TX_DMA_Stream);

        if (i2s.tx_callback) {
            i2s.tx_callback(i2s.tx_context);
        }
    }
}

inline static
int
loadStm32f2xxI2sConfigFromHalI2sConfig (
    I2S_InitTypeDef * stm32f2xx_i2s_config_,
    hal_i2s_config_t * hal_i2s_config_
) {
    // Convert data format
    switch (hal_i2s_config_->data_format) {
      case HAL_I2S_DATA_FORMAT_16B:
        stm32f2xx_i2s_config_->I2S_DataFormat = I2S_DataFormat_16b;
        break;
      case HAL_I2S_DATA_FORMAT_16B_EXTENDED:
        stm32f2xx_i2s_config_->I2S_DataFormat = I2S_DataFormat_16bextended;
        break;
      case HAL_I2S_DATA_FORMAT_24B:
        stm32f2xx_i2s_config_->I2S_DataFormat = I2S_DataFormat_24b;
        break;
      case HAL_I2S_DATA_FORMAT_32B:
        stm32f2xx_i2s_config_->I2S_DataFormat = I2S_DataFormat_32b;
        break;
      default:
        return 1;
    }

    // Convert audio frequency
    switch (hal_i2s_config_->frequency) {
      case HAL_I2S_AUDIO_FREQUENCY_8K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_8k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_11K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_11k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_16K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_16k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_22K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_22k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_32K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_32k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_44K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_44k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_48K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_48k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_96K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_96k;
        break;
      case HAL_I2S_AUDIO_FREQUENCY_192K:
        stm32f2xx_i2s_config_->I2S_AudioFreq = I2S_AudioFreq_192k;
        break;
      default:
        return 1;
    }

    // Convert mode (only Master TX is supported)
    switch (hal_i2s_config_->mode) {
      case HAL_I2S_MODE_MASTER_TX:
        stm32f2xx_i2s_config_->I2S_Mode = I2S_Mode_MasterTx;
        break;
      case HAL_I2S_MODE_MASTER_RX:
        //stm32f2xx_i2s_config_->I2S_Mode = I2S_Mode_MasterRx;
        //break;
      case HAL_I2S_MODE_SLAVE_TX:
        //stm32f2xx_i2s_config_->I2S_Mode = I2S_Mode_SlaveTx;
        //break;
      case HAL_I2S_MODE_SLAVE_RX:
        //stm32f2xx_i2s_config_->I2S_Mode = I2S_Mode_SlaveRx;
        //break;
      default:
        return 1;
    }

    // Convert polarity
    switch (hal_i2s_config_->polarity) {
      case HAL_I2S_CLOCK_POLARITY_HIGH:
        stm32f2xx_i2s_config_->I2S_CPOL = I2S_CPOL_High;
        break;
      case HAL_I2S_CLOCK_POLARITY_LOW:
        stm32f2xx_i2s_config_->I2S_CPOL = I2S_CPOL_Low;
        break;
      default:
        return 1;
    }

    // Convert standard
    switch (hal_i2s_config_->standard) {
      case HAL_I2S_STANDARD_LSB:
        stm32f2xx_i2s_config_->I2S_Standard = I2S_Standard_LSB;
        break;
      case HAL_I2S_STANDARD_MSB:
        stm32f2xx_i2s_config_->I2S_Standard = I2S_Standard_MSB;
        break;
      case HAL_I2S_STANDARD_PCM_LONG:
        stm32f2xx_i2s_config_->I2S_Standard = I2S_Standard_PCMLong;
        break;
      case HAL_I2S_STANDARD_PCM_SHORT:
        stm32f2xx_i2s_config_->I2S_Standard = I2S_Standard_PCMShort;
        break;
      case HAL_I2S_STANDARD_PHILIPS:
        stm32f2xx_i2s_config_->I2S_Standard = I2S_Standard_Phillips;
        break;
      default:
        return 1;
    }

    // Enable Master Clock Output
    stm32f2xx_i2s_config_->I2S_MCLKOutput = I2S_MCLKOutput_Disable;

    return 0;
}

int
HAL_I2S_Begin (
    HAL_I2S_Interface interface_,
    hal_i2s_config_t * config_
) {
    // Validate paramters
    if (interface_ != i2s.active_interface) {
    } else if (!i2s.initialized) {
    } else if (i2s.ready) {
    } else if (!config_ || loadStm32f2xxI2sConfigFromHalI2sConfig(&i2s.i2s_config, config_)) {

    // Configure I2S
    } else {
        // Enable I2S Peripheral
        RCC_APB1PeriphResetCmd(i2s_req.I2S_RCC_APB1Periph, ENABLE);

        // Load Pin Mapping
        STM32_Pin_Info* PIN_MAP = HAL_Pin_Map();

        // Connect I2S pins to AF
        GPIO_PinAFConfig(PIN_MAP[i2s_req.i2s_pins[i2s.active_interface].I2S_SCK_Pin].gpio_peripheral, PIN_MAP[i2s_req.i2s_pins[i2s.active_interface].I2S_SCK_Pin].gpio_pin_source, i2s_req.I2S_AF_Mapping);
        GPIO_PinAFConfig(PIN_MAP[i2s_req.i2s_pins[i2s.active_interface].I2S_SD_Pin].gpio_peripheral, PIN_MAP[i2s_req.i2s_pins[i2s.active_interface].I2S_SD_Pin].gpio_pin_source, i2s_req.I2S_AF_Mapping);
        GPIO_PinAFConfig(PIN_MAP[i2s_req.i2s_pins[i2s.active_interface].I2S_WS_Pin].gpio_peripheral, PIN_MAP[i2s_req.i2s_pins[i2s.active_interface].I2S_WS_Pin].gpio_pin_source, i2s_req.I2S_AF_Mapping);

        HAL_Pin_Mode(i2s_req.i2s_pins[i2s.active_interface].I2S_SCK_Pin, AF_OUTPUT_PUSHPULL);
        HAL_Pin_Mode(i2s_req.i2s_pins[i2s.active_interface].I2S_SD_Pin, AF_OUTPUT_PUSHPULL);
        HAL_Pin_Mode(i2s_req.i2s_pins[i2s.active_interface].I2S_WS_Pin, AF_OUTPUT_PUSHPULL);

        // Enable I2S PLL Clock
        RCC_I2SCLKConfig(RCC_I2S2CLKSource_PLLI2S);
        RCC_PLLI2SCmd(ENABLE);
        for (; SET != RCC_GetFlagStatus(RCC_FLAG_PLLI2SRDY) ; /*os_thread_yield()*/);

        // Initialize the I2S Bus
        I2S_Init(i2s_req.I2S_Peripheral, &i2s.i2s_config);

        // Select all the potential interruption sources and
        // the DMA capabilities by writing the SPI_CR2 register

        // Enable DMA Controller Clock
        RCC_AHB1PeriphResetCmd(i2s_req.DMA_RCC_AHBRegister, ENABLE);

        // Setup the NVIC (see table in misc.c header)
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
        i2s.nvic_config.NVIC_IRQChannelPreemptionPriority = 12;
        i2s.nvic_config.NVIC_IRQChannelSubPriority = 0;
        i2s.nvic_config.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&i2s.nvic_config);

        // Enable SPI "Transfer Complete" DMA Stream Interrupt
        DMA_ITConfig(i2s_req.I2S_EXT_TX_DMA_Stream, DMA_IT_TC, ENABLE);

        i2s.ready = true;
        return 0;
    }

    return 1;
}

void
HAL_I2S_End (
    HAL_I2S_Interface interface_
) {
    // Validate paramters
    if (interface_ != i2s.active_interface) {
    } else if (!i2s.ready) {

    // Disable I2S
    } else {
        // Disable the I2S Bus (it is mandatory to wait for TXE = 1 and BSY = 0)
        //TODO: for (; SET != SPI_I2S_GetFlagStatus(i2s_req.I2S_Peripheral, SPI_I2S_FLAG_TXE) ; /*os_thread_yield()*/);
        for (; RESET != SPI_I2S_GetFlagStatus(i2s_req.I2S_Peripheral, SPI_I2S_FLAG_BSY) ; /*os_thread_yield()*/);

        // Release DMA and peripheral resources
        DMA_Cmd(i2s_req.I2S_EXT_TX_DMA_Stream, DISABLE);
        I2S_Cmd(i2s_req.I2S_Peripheral, DISABLE);
        SPI_I2S_DMACmd(i2s_req.I2S_Peripheral, SPI_I2S_DMAReq_Tx, DISABLE);
        SPI_I2S_DeInit(i2s_req.I2S_Peripheral);

        // Disable SPI "Transfer Complete" DMA Stream Interrupt
        DMA_ITConfig(i2s_req.I2S_EXT_TX_DMA_Stream, DMA_IT_TC, DISABLE);

        // Disable the Selected IRQ Channels
        i2s.nvic_config.NVIC_IRQChannelCmd = DISABLE;
        NVIC_Init(&i2s.nvic_config);

        // Disable DMA Controller Clock
        RCC_AHB1PeriphResetCmd(i2s_req.DMA_RCC_AHBRegister, DISABLE);

        // Disable I2S PLL Clock
        RCC_PLLI2SCmd(DISABLE);

        // Return pins to INPUT mode
        HAL_Pin_Mode(i2s_req.i2s_pins[i2s.active_interface].I2S_SCK_Pin, INPUT);
        HAL_Pin_Mode(i2s_req.i2s_pins[i2s.active_interface].I2S_SD_Pin, INPUT);
        HAL_Pin_Mode(i2s_req.i2s_pins[i2s.active_interface].I2S_WS_Pin, INPUT);

        // Disable I2S Peripheral
        RCC_APB1PeriphResetCmd(i2s_req.I2S_RCC_APB1Periph, DISABLE);
    }
    i2s.ready = false;
}

int
HAL_I2S_Init (
    HAL_I2S_Interface interface_
) {
    // Validate paramters
    if (interface_ >= TOTAL_I2S) {

    // Initialize state
    } else {
        I2S_StructInit(&i2s.i2s_config);

        // Initialize DMA Init Struct
        DMA_StructInit(&i2s.dma_config);
        i2s.dma_config.DMA_Channel = i2s_req.I2S_DMA_Channel;
        i2s.dma_config.DMA_DIR = DMA_DIR_MemoryToPeripheral;
        i2s.dma_config.DMA_FIFOMode = DMA_FIFOMode_Disable;  // Direct mode
        i2s.dma_config.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;  // Has no effect when FIFO mode is disabled
        i2s.dma_config.DMA_MemoryBurst = DMA_MemoryBurst_Single;
        i2s.dma_config.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
        i2s.dma_config.DMA_MemoryInc = DMA_MemoryInc_Enable;
        i2s.dma_config.DMA_Mode = DMA_Mode_Normal;  // Non-circular
        i2s.dma_config.DMA_PeripheralBaseAddr = i2s_req.I2S_Peripheral->DR;
        i2s.dma_config.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;  // Has no effect when peripheral increment is disabled
        i2s.dma_config.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        i2s.dma_config.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        i2s.dma_config.DMA_Priority = DMA_Priority_High;

        // Initialize NVIC Init Struct
        i2s.nvic_config.NVIC_IRQChannel = i2s_req.I2S_EXT_TX_DMA_Stream_IRQn;
        i2s.nvic_config.NVIC_IRQChannelCmd = ENABLE;
        i2s.nvic_config.NVIC_IRQChannelPreemptionPriority = 12;
        i2s.nvic_config.NVIC_IRQChannelSubPriority = 0;

        i2s.initialized = true;
        i2s.ready = false;
        i2s.tx_callback = (hal_i2s_callback_t)NULL;
        i2s.tx_context = NULL;

        // Identify active interface
        i2s.active_interface = interface_;

        return 0;
    }

    return 1;
}

uint32_t
HAL_I2S_Transmit (
    HAL_I2S_Interface interface_,
    uint16_t * buffer_,
    size_t buffer_size_,
    hal_i2s_callback_t tx_callback_,
    void * tx_context_
) {
    // Check for ready state
    if (!i2s.ready) {

    // Validate paramters
    } else if (interface_ != i2s.active_interface) {
    } else if (!buffer_) {

    // DMA flow controller: the number of data items to be
    // transferred is software-programmable from 1 to 65535
    } else if (!buffer_size_ || buffer_size_ > 65535) {

    // Check for unbalanced L/R channel data (incomplete transaction)
    } else if (buffer_size_ % 2) {
    } else if ((I2S_DataFormat_16b != i2s.i2s_config.I2S_DataFormat) && (buffer_size_ % 4)) {

    // Invoke I2S
    } else {
        // Clear context if no callback was provided
        if (!tx_callback_) {
            i2s.tx_context = NULL;
        }

        // Record callback and context
        i2s.tx_callback = tx_callback_;
        i2s.tx_context = tx_context_;

        // Describe the data buffer
        i2s.dma_config.DMA_Memory0BaseAddr = (uint32_t)buffer_;
        i2s.dma_config.DMA_BufferSize = buffer_size_;

//============================================
//============= Transmit buffer ==============
//============================================
//== procedure specified in stm32f2xx_spi.c ==
//============================================
        int is = HAL_disable_irq();

        // Update DMA configuration

        // The DMA_Init() function follows the DMA configuration procedures
        // as described in reference manual (RM0033) except the first point:
        // waiting on EN bit to be reset. This condition should be checked by
        // user application using the function DMA_GetCmdStatus() before
        // calling the DMA_Init() function.
        DMA_DeInit(i2s_req.I2S_EXT_TX_DMA_Stream);
        for (; ENABLE == DMA_GetCmdStatus(i2s_req.I2S_EXT_TX_DMA_Stream) ; /*os_thread_yield()*/);
        DMA_Init(i2s_req.I2S_EXT_TX_DMA_Stream, &i2s.dma_config);

        // Enable the I2S Tx DMA request (transmit buffer)
        SPI_I2S_DMACmd(i2s_req.I2S_Peripheral, SPI_I2S_DMAReq_Tx, ENABLE);

        // Enable the I2S Bus
        I2S_Cmd(i2s_req.I2S_Peripheral, ENABLE);

        // Enable the DMA Tx Stream
        DMA_Cmd(i2s_req.I2S_EXT_TX_DMA_Stream, ENABLE);

        HAL_enable_irq(is);
//============================================

        // Poll for transfer complete flag (block) if no callback was provided
        if (!tx_callback_) {
            for (; RESET != SPI_I2S_GetFlagStatus(i2s_req.I2S_Peripheral, SPI_I2S_FLAG_BSY) ; /*os_thread_yield()*/);
        }

        // Indicate bytes sent
        return buffer_size_;
    }

    return 0;
}

/* Created and copyrighted by Zachary J. Fields. Offered as open source under the MIT License (MIT). */
