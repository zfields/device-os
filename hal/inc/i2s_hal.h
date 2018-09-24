/* Created and copyrighted by Zachary J. Fields. Offered as open source under the MIT License (MIT). */

#ifndef __I2S_HAL_H
#define	__I2S_HAL_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    HAL_I2S_INTERFACE1 = 0     // maps to I2S3 (pins: D4, D2, D5)
#if PLATFORM_ID == 10 // Electron
   ,HAL_I2S_INTERFACE2 = 1     // maps to I2S3 (pins: C3, C1, A6)
#endif
} HAL_I2S_Interface;

typedef enum {
    HAL_I2S_AUDIO_FREQUENCY_8K,
    HAL_I2S_AUDIO_FREQUENCY_11K,
    HAL_I2S_AUDIO_FREQUENCY_16K,
    HAL_I2S_AUDIO_FREQUENCY_22K,
    HAL_I2S_AUDIO_FREQUENCY_32K,
    HAL_I2S_AUDIO_FREQUENCY_44K,
    HAL_I2S_AUDIO_FREQUENCY_48K,
    HAL_I2S_AUDIO_FREQUENCY_96K,
    HAL_I2S_AUDIO_FREQUENCY_192K
} HAL_I2S_Audio_Frequency;

typedef enum {
    HAL_I2S_CLOCK_POLARITY_LOW,
    HAL_I2S_CLOCK_POLARITY_HIGH
} HAL_I2S_Clock_Polarity;

typedef enum {
    HAL_I2S_DATA_FORMAT_16B,
    HAL_I2S_DATA_FORMAT_16B_EXTENDED,
    HAL_I2S_DATA_FORMAT_24B,
    HAL_I2S_DATA_FORMAT_32B
} HAL_I2S_Data_Format;

typedef enum {
    HAL_I2S_MODE_MASTER_TX,
    HAL_I2S_MODE_MASTER_RX,
    HAL_I2S_MODE_SLAVE_TX,
    HAL_I2S_MODE_SLAVE_RX
} HAL_I2S_Mode;

typedef enum {
    HAL_I2S_STANDARD_PHILIPS,
    HAL_I2S_STANDARD_MSB,
    HAL_I2S_STANDARD_LSB,
    HAL_I2S_STANDARD_PCM_SHORT,
    HAL_I2S_STANDARD_PCM_LONG
} HAL_I2S_Standard;

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hal_i2s_callback_t)(void * context_);

typedef struct hal_i2s_config_t {
    HAL_I2S_Mode mode;
    HAL_I2S_Standard standard;
    HAL_I2S_Data_Format data_format;
    HAL_I2S_Audio_Frequency frequency;
    HAL_I2S_Clock_Polarity polarity;
} hal_i2s_config_t;

int
HAL_I2S_Begin (
    HAL_I2S_Interface i2s_,
    const hal_i2s_config_t * config_
);

void
HAL_I2S_End (
    HAL_I2S_Interface i2s_
);

int
HAL_I2S_Init (
    HAL_I2S_Interface i2s_
);

uint32_t
HAL_I2S_Transmit (
    HAL_I2S_Interface i2s_,
    const uint16_t * buffer_,
    size_t buffer_size_,
    hal_i2s_callback_t tx_callback_,
    void * tx_context_
);

#ifdef __cplusplus
}
#endif

#endif	/* __I2S_HAL_H */

/* Created and copyrighted by Zachary J. Fields. Offered as open source under the MIT License (MIT). */
