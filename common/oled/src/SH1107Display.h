/**
 * @file SH1107Display.h
 * @brief Compatibility shim for SH1107 OLED controller
 * 
 * Provides a compatibility layer that maps SH1107 symbol names to the CH1115
 * driver implementation. The SH1107 controller is functionally similar to CH1115
 * in this codebase, allowing projects to use either controller with the same
 * driver implementation.
 * 
 * 
 * @note SH1107 typically uses I2C address 0x3D (default), while CH1115 uses 0x3C
 */

#ifndef _SH1107_DISPLAY_H
#define _SH1107_DISPLAY_H

#include "CH1115Display.h"

/** @defgroup SH1107_Compatibility Compatibility Macros for SH1107
 *  @{
 *  @brief Symbol mappings from SH1107 namespace to CH1115 implementation
 */

/** @brief Default I2C address for SH1107 (0x3D) */
#define SH1107_I2C_ADDRESS 0x3D
/** @brief SH1107 device identifier */
#define SH1107_DEVICE_ID CH1115_DEVICE_ID

// Color definitions mapped to CH1115
/** @brief White pixel */
#define SH1107_WHITE_COLOR CH1115_WHITE_COLOR
/** @brief Black pixel */
#define SH1107_BLACK_COLOR CH1115_BLACK_COLOR
/** @brief Inverted pixel */
#define SH1107_INVERSE_COLOR CH1115_INVERSE_COLOR

// On/Off states
/** @brief OFF state */
#define SH1107_OFF CH1115_OFF
/** @brief ON state */
#define SH1107_ON CH1115_ON

// Scrolling definitions
/** @brief Scroll right direction */
#define SH1107_SCROLL_RIGHT CH1115_SCROLL_RIGHT
/** @brief Scroll left direction */
#define SH1107_SCROLL_LEFT CH1115_SCROLL_LEFT
/** @brief Disable scrolling */
#define SH1107_SCROLL_OFF CH1115_SCROLL_OFF
/** @brief Continuous scrolling mode */
#define SH1107_SCROLL_CONTINUOUS CH1115_SCROLL_CONTINUOUS
/** @brief Single screen scroll mode */
#define SH1107_SCROLL_ONCE CH1115_SCROLL_ONCE
/** @brief One column scroll mode */
#define SH1107_SCROLL_ONE_COLUMN CH1115_SCROLL_ONE_COLUMN

// Frame intervals for scrolling
/** @brief 2-frame scroll interval */
#define SH1107_SCROLL_2FRAMES CH1115_SCROLL_2FRAMES
/** @brief 3-frame scroll interval */
#define SH1107_SCROLL_3FRAMES CH1115_SCROLL_3FRAMES
/** @brief 4-frame scroll interval */
#define SH1107_SCROLL_4FRAMES CH1115_SCROLL_4FRAMES
/** @brief 5-frame scroll interval */
#define SH1107_SCROLL_5FRAMES CH1115_SCROLL_5FRAMES
/** @brief 6-frame scroll interval */
#define SH1107_SCROLL_6FRAMES CH1115_SCROLL_6FRAMES
/** @brief 32-frame scroll interval */
#define SH1107_SCROLL_32FRAMES CH1115_SCROLL_32FRAMES
/** @brief 64-frame scroll interval */
#define SH1107_SCROLL_64FRAMES CH1115_SCROLL_64FRAMES
/** @brief 128-frame scroll interval */
#define SH1107_SCROLL_128FRAMES CH1115_SCROLL_128FRAMES

// Drawing modes
/** @brief Overwrite drawing mode */
#define SH1107_OVERWRITE_MODE OVERWRITE_MODE
/** @brief OR drawing mode */
#define SH1107_OR_MODE OR_MODE
/** @brief XOR drawing mode */
#define SH1107_XOR_MODE XOR_MODE
/** @brief AND drawing mode */
#define SH1107_AND_MODE AND_MODE

/** @} */

/**
 * @brief SH1107Display class type alias
 * 
 * Type alias that exposes the CH1115 driver as SH1107Display for code compatibility.
 * Projects using SH1107 controllers can instantiate SH1107Display objects
 * which will use the shared CH1115 implementation.
 */
using SH1107Display = CH1115Display;

#endif // _SH1107_DISPLAY_H
