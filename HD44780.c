/**
 * @file HD44780.c
 * @brief Implementation of the HD44780 LCD library.
 *
 * @author Eziya - Daniel Ruiz
 * @date April 30, 2026
 * @version 2.0.0
 */

#include "HD44780.h"

// ============================================================================
// VARIABLES PRIVADAS
// ============================================================================

static I2C_HandleTypeDef* HD44780_hi2c        = NULL;
static uint8_t            HD44780_Initialized = 0;

static uint8_t dpFunction;
static uint8_t dpControl;
static uint8_t dpMode;
static uint8_t dpRows;
static uint8_t dpBacklight;

static uint8_t special1[8] = {
        0b00000,
        0b11001,
        0b11011,
        0b00110,
        0b01100,
        0b11011,
        0b10011,
        0b00000
};

static uint8_t special2[8] = {
        0b11000,
        0b11000,
        0b00110,
        0b01001,
        0b01000,
        0b01001,
        0b00110,
        0b00000
};

static uint8_t heart[8] = {
        0b00000,
        0b01010,
        0b11111,
        0b11111,
        0b01110,
        0b00100,
        0b00000,
        0b00000
};

static uint8_t Cyrilic[8] = {0x1F, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E, 0x00};

static uint8_t Flecha[8] = {
        0b00000,
        0b00100,
        0b00110,
        0b11111,
        0b00110,
        0b00100,
        0b00000,
        0b00000
};

static uint8_t Campana[8] = {
        0b00100,
        0b01110,
        0b01110,
        0b01110,
        0b11111,
        0b00000,
        0b00100,
        0b00000
};

static uint8_t degrees[8] = {
        0b11100,
        0b10100,
        0b11100,
        0b00000,
        0b00000,
        0b00000,
        0b00000,
        0b00000
};

// ============================================================================
// PROTOTIPOS DE FUNCIONES PRIVADAS
// ============================================================================

static void             DelayUS(uint32_t us);
static void             DelayInit(void);
static HD44780_Status_t ExpanderWrite(uint8_t data);
static HD44780_Status_t PulseEnable(uint8_t data);
static HD44780_Status_t Write4Bits(uint8_t value);
static HD44780_Status_t Send(uint8_t value, uint8_t mode);
static HD44780_Status_t SendCommand(uint8_t cmd);
static HD44780_Status_t SendChar(uint8_t ch);

// ============================================================================
// FUNCIONES PRIVADAS
// ============================================================================

static void DelayUS(uint32_t us)
{
    uint32_t cycles = (SystemCoreClock / 1000000L) * us;
    uint32_t start  = DWT->CYCCNT;
    volatile uint32_t cnt;

    do
    {
        cnt = DWT->CYCCNT - start;
    } while (cnt < cycles);
}

static void DelayInit(void)
{
    CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
    CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk;

    DWT->CYCCNT = 0;

    __ASM volatile ("NOP");
    __ASM volatile ("NOP");
    __ASM volatile ("NOP");
}

static HD44780_Status_t ExpanderWrite(uint8_t _data)
{
    uint8_t data = _data | dpBacklight;
    HAL_StatusTypeDef halStatus = HAL_I2C_Master_Transmit(
        HD44780_hi2c, HD44780_ADDRESS, (uint8_t*)&data, 1U, HD44780_TIMEOUT_MS
    );
    if (halStatus == HAL_TIMEOUT) { return HD44780_TIMEOUT; }
    if (halStatus != HAL_OK)      { return HD44780_ERROR;   }
    return HD44780_OK;
}

static HD44780_Status_t PulseEnable(uint8_t _data)
{
    HD44780_Status_t status;

    status = ExpanderWrite(_data | ENABLE);
    if (status != HD44780_OK) { return status; }
    DelayUS(20);

    status = ExpanderWrite(_data & ~ENABLE);
    if (status != HD44780_OK) { return status; }
    DelayUS(20);

    return HD44780_OK;
}

static HD44780_Status_t Write4Bits(uint8_t value)
{
    HD44780_Status_t status;

    status = ExpanderWrite(value);
    if (status != HD44780_OK) { return status; }
    return PulseEnable(value);
}

static HD44780_Status_t Send(uint8_t value, uint8_t mode)
{
    uint8_t highnib = value & 0xF0;
    uint8_t lownib  = (value << 4) & 0xF0;
    HD44780_Status_t status;

    status = Write4Bits(highnib | mode);
    if (status != HD44780_OK)
	{ 
		return status;
	}

    return Write4Bits(lownib | mode);
}

static HD44780_Status_t SendCommand(uint8_t cmd)
{
    return Send(cmd, 0);
}

static HD44780_Status_t SendChar(uint8_t ch)
{
    return Send(ch, RS);
}

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

/**
 * @brief Inicializa el módulo HD44780 con el handle I2C.
 *
 * @param[in] hi2c Puntero al handle de I2C.
 * @param[in] rows Número de filas del display (1 o 2).
 * @return HD44780_Status_t Estado de la inicialización (OK, ERROR, etc.)
 */
HD44780_Status_t HD44780_Init(I2C_HandleTypeDef* hi2c, uint8_t rows)
{
    if(hi2c == NULL)
	{
		return HD44780_INVALID_PARAM;
	}
    if(rows == 0)
	{ 
		return HD44780_INVALID_PARAM;
	}

    HD44780_hi2c = hi2c;
    dpRows       = rows;
    dpBacklight  = LCD_BACKLIGHT;
    dpFunction   = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

    if (dpRows > 1) { dpFunction |= LCD_2LINE;    }
    else            { dpFunction |= LCD_5x10DOTS; }

    DelayInit();
    HAL_Delay(50);

    HD44780_Status_t status;
    status = ExpanderWrite(dpBacklight);
    if (status != HD44780_OK) { return status; }
    HAL_Delay(1000);

    /* Secuencia de modo 4-bit — sensible al timing, errores no propagados */
    Write4Bits(0x03 << 4);
    DelayUS(4500);
    Write4Bits(0x03 << 4);
    DelayUS(4500);
    Write4Bits(0x03 << 4);
    DelayUS(4500);
    Write4Bits(0x02 << 4);
    DelayUS(100);

    status = SendCommand(LCD_FUNCTIONSET | dpFunction);
    if (status != HD44780_OK) { return status; }

    dpControl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    status = SendCommand(LCD_DISPLAYCONTROL | dpControl);
    if (status != HD44780_OK) { return status; }

    status = SendCommand(LCD_CLEARDISPLAY);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);

    dpMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    status = SendCommand(LCD_ENTRYMODESET | dpMode);
    if (status != HD44780_OK) { return status; }
    DelayUS(4500);

    HD44780_CreateSpecialChar(0, special1);
    HD44780_CreateSpecialChar(1, special2);
    HD44780_CreateSpecialChar(2, heart);
    HD44780_CreateSpecialChar(3, Cyrilic);
    HD44780_CreateSpecialChar(4, Flecha);
    HD44780_CreateSpecialChar(5, Campana);
    HD44780_CreateSpecialChar(6, degrees);

    status = SendCommand(LCD_RETURNHOME);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);

    HD44780_Initialized = 1U;

    return HD44780_OK;
}

HD44780_Status_t HD44780_Clear(void)
{
    HD44780_Status_t status = SendCommand(LCD_CLEARDISPLAY);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);
    return HD44780_OK;
}

HD44780_Status_t HD44780_Home(void)
{
    HD44780_Status_t status = SendCommand(LCD_RETURNHOME);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);
    return HD44780_OK;
}

HD44780_Status_t HD44780_SetCursor(uint8_t col, uint8_t row)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    const int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row >= dpRows) { row = dpRows - 1; }
    return SendCommand(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

HD44780_Status_t HD44780_NoDisplay(void)
{
    dpControl &= ~LCD_DISPLAYON;
    return SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_Display(void)
{
    dpControl |= LCD_DISPLAYON;
    return SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_NoCursor(void)
{
    dpControl &= ~LCD_CURSORON;
    return SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_Cursor(void)
{
    dpControl |= LCD_CURSORON;
    return SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_NoBlink(void)
{
    dpControl &= ~LCD_BLINKON;
    return SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_Blink(void)
{
    dpControl |= LCD_BLINKON;
    return SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_ScrollDisplayLeft(void)
{
    return SendCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

HD44780_Status_t HD44780_ScrollDisplayRight(void)
{
    return SendCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

HD44780_Status_t HD44780_LeftToRight(void)
{
    dpMode |= LCD_ENTRYLEFT;
    return SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_RightToLeft(void)
{
    dpMode &= ~LCD_ENTRYLEFT;
    return SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_AutoScroll(void)
{
    dpMode |= LCD_ENTRYSHIFTINCREMENT;
    return SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_NoAutoScroll(void)
{
    dpMode &= ~LCD_ENTRYSHIFTINCREMENT;
    return SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_CreateSpecialChar(uint8_t location, uint8_t charmap[])
{
    if (charmap == NULL) { return HD44780_INVALID_PARAM; }

    HD44780_Status_t status;
    location &= 0x7;
    status = SendCommand(LCD_SETCGRAMADDR | (location << 3));
    if (status != HD44780_OK) { return status; }

    for (int i = 0; i < 8; i++)
    {
        status = SendChar(charmap[i]);
        if (status != HD44780_OK) { return status; }
    }

    return HD44780_OK;
}

HD44780_Status_t HD44780_PrintSpecialChar(uint8_t index)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }
    return SendChar(index);
}

HD44780_Status_t HD44780_LoadCustomCharacter(uint8_t char_num, uint8_t *rows)
{
    if (rows == NULL) { return HD44780_INVALID_PARAM; }
    return HD44780_CreateSpecialChar(char_num, rows);
}

HD44780_Status_t HD44780_PrintStr(const char c[])
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }
    if (c == NULL)                 { return HD44780_INVALID_PARAM;   }

    HD44780_Status_t status;
    while (*c)
    {
        status = SendChar((uint8_t)*c++);
        if (status != HD44780_OK) { return status; }
    }

    return HD44780_OK;
}

HD44780_Status_t HD44780_SetBacklight(uint8_t new_val)
{
    if (new_val) { return HD44780_Backlight();   }
    else         { return HD44780_NoBacklight(); }
}

HD44780_Status_t HD44780_NoBacklight(void)
{
    dpBacklight = LCD_NOBACKLIGHT;
    return ExpanderWrite(0);
}

HD44780_Status_t HD44780_Backlight(void)
{
    dpBacklight = LCD_BACKLIGHT;
    return ExpanderWrite(0);
}
