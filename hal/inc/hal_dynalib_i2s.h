/**
 ******************************************************************************
 * @file    hal_dynalib_i2s.h
 * @authors Zachary J. Fields
 * @date    23 September 2018
 ******************************************************************************
  Copyright (c) 2015 Particle Industries, Inc.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

#ifndef HAL_DYNALIB_I2S_H
#define	HAL_DYNALIB_I2S_H

#include "dynalib.h"

#ifdef DYNALIB_EXPORT
#include "i2s_hal.h"
#endif

// WARNING
// The order of functions must not be changed or older applications will break
// when used with newer system firmware.
// Function signatures shouldn't be changed other than changing pointer types.
// New HAL functions must be added to the end of this list.
// GNINRAW

DYNALIB_BEGIN(hal_i2s)

DYNALIB_FN(0, hal_i2s, HAL_I2S_Begin, int(HAL_I2S_Interface, const hal_i2s_config_t *))
DYNALIB_FN(1, hal_i2s, HAL_I2S_End, void(HAL_I2S_Interface))
DYNALIB_FN(2, hal_i2s, HAL_I2S_Init, int(HAL_I2S_Interface))
DYNALIB_FN(3, hal_i2s, HAL_I2S_Transmit, uint32_t(HAL_I2S_Interface, const uint16_t *, size_t, hal_i2s_callback_t, void *))

DYNALIB_END(hal_i2s)

#endif	/* HAL_DYNALIB_I2S_H */
