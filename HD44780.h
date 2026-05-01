/**
 * @file HD44780.h
 * @brief Librería para la gestión del módulo LCD HD44780 mediante comunicación I2C.
 *
 * @author Eziya - Daniel Ruiz
 * @date April 30, 2026
 * @version 2.0.0
 */

#ifndef HD44780_H
#define HD44780_H

// ============================================================================
// INCLUDES
// ============================================================================

/* Ajustar según la familia STM32 utilizada:
 * STM32F0xx: #include "stm32f0xx_hal.h"
 * STM32F1xx: #include "stm32f1xx_hal.h"
 * STM32F4xx: #include "stm32f4xx_hal.h"
 */
#include "stm32f1xx_hal.h"

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// MACROS Y CONSTANTES DE COMANDOS HD44780
// ============================================================================

/** @brief Dirección I2C del módulo LCD HD44780. */
#define HD44780_ADDRESS     (0x27 << 1)     /**< I2C device address (0x4E) */

/** @brief Timeout por defecto para transmisiones HAL (ms). */
#define HD44780_TIMEOUT_MS  10U

#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

/* Entry Mode */
#define LCD_ENTRYRIGHT              0x00
#define LCD_ENTRYLEFT               0x02
#define LCD_ENTRYSHIFTINCREMENT     0x01
#define LCD_ENTRYSHIFTDECREMENT     0x00

/* Display On/Off */
#define LCD_DISPLAYON   0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON    0x02
#define LCD_CURSOROFF   0x00
#define LCD_BLINKON     0x01
#define LCD_BLINKOFF    0x00

/* Cursor Shift */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT   0x04
#define LCD_MOVELEFT    0x00

/* Function Set */
#define LCD_8BITMODE    0x10
#define LCD_4BITMODE    0x00
#define LCD_2LINE       0x08
#define LCD_1LINE       0x00
#define LCD_5x10DOTS    0x04
#define LCD_5x8DOTS     0x00

/* Backlight */
#define LCD_BACKLIGHT   0x08
#define LCD_NOBACKLIGHT 0x00

/* Enable Bit */
#define ENABLE  0x04

/* Read/Write Bit */
#define RW  0x02

/* Register Select Bit */
#define RS  0x01

// ============================================================================
// ENUMERACIONES Y ESTRUCTURAS
// ============================================================================

/**
 * @brief Códigos de estado retornados por todas las funciones públicas del HD44780.
 */
typedef enum {
    HD44780_OK              = 0,    /**< Operación exitosa */
    HD44780_ERROR           = 1,    /**< Error en la operación */
    HD44780_TIMEOUT         = 2,    /**< Timeout en la operación */
    HD44780_NOT_INITIALIZED = 3,    /**< Módulo no inicializado */
    HD44780_INVALID_PARAM   = 4,    /**< Parámetro inválido */
} HD44780_Status_t;

// ============================================================================
// PROTOTIPOS DE FUNCIONES PÚBLICAS
// ============================================================================

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Inicializa el módulo HD44780 con el handle I2C.
 *
 * @param[in] hi2c Puntero al handle de I2C.
 * @param[in] rows Número de filas del display (1 o 2).
 * @return HD44780_Status_t
 *         - HD44780_OK            si la inicialización fue exitosa.
 *         - HD44780_ERROR         si ocurrió un error de comunicación.
 *         - HD44780_INVALID_PARAM si @p hi2c es NULL o @p rows es 0.
 */
HD44780_Status_t HD44780_Init(I2C_HandleTypeDef* hi2c, uint8_t rows);

HD44780_Status_t HD44780_Clear(void);
HD44780_Status_t HD44780_Home(void);
HD44780_Status_t HD44780_NoDisplay(void);
HD44780_Status_t HD44780_Display(void);
HD44780_Status_t HD44780_NoBlink(void);
HD44780_Status_t HD44780_Blink(void);
HD44780_Status_t HD44780_NoCursor(void);
HD44780_Status_t HD44780_Cursor(void);
HD44780_Status_t HD44780_ScrollDisplayLeft(void);
HD44780_Status_t HD44780_ScrollDisplayRight(void);
HD44780_Status_t HD44780_LeftToRight(void);
HD44780_Status_t HD44780_RightToLeft(void);
HD44780_Status_t HD44780_NoBacklight(void);
HD44780_Status_t HD44780_Backlight(void);
HD44780_Status_t HD44780_AutoScroll(void);
HD44780_Status_t HD44780_NoAutoScroll(void);
HD44780_Status_t HD44780_CreateSpecialChar(uint8_t location, uint8_t charmap[]);
HD44780_Status_t HD44780_PrintSpecialChar(uint8_t index);
HD44780_Status_t HD44780_SetCursor(uint8_t col, uint8_t row);
HD44780_Status_t HD44780_SetBacklight(uint8_t new_val);
HD44780_Status_t HD44780_LoadCustomCharacter(uint8_t char_num, uint8_t *rows);
HD44780_Status_t HD44780_PrintStr(const char c[]);

#ifdef __cplusplus
}
#endif

#endif /* HD44780_H */
