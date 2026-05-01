/**
 * @file HD44780.h
 * @brief Librería para la gestión del módulo LCD HD44780 mediante comunicación I2C.
 *
 * @author Eziya - Daniel Ruiz
 * @date April 30, 2026
 * @version 2.0.0
 */

#include "HD44780.h"

// ============================================================================
// VARIABLES PRIVADAS
// ============================================================================

static I2C_HandleTypeDef* HD44780_hi2c  =   NULL;  /**< Manejador de la interfaz I2C utilizado para comunicarse con el RTC */
static uint8_t     HD44780_Initialized  =   0;

// ============================================================================
// FUNCIONES PRIVADAS
// ============================================================================/**< Bandera para verificar si el módulo está inicializado */

uint8_t dpFunction;
uint8_t dpControl;
uint8_t dpMode;
uint8_t dpRows;
uint8_t dpBacklight;

uint8_t special1[8] = {
		0b00000,
		0b11001,
		0b11011,
		0b00110,
		0b01100,
		0b11011,
		0b10011,
		0b00000
};

uint8_t special2[8] = {
		0b11000,
		0b11000,
		0b00110,
		0b01001,
		0b01000,
		0b01001,
		0b00110,
		0b00000
};

uint8_t heart[8]   = {
		0b000000,
		0b01010,
		0b11111,
		0b11111,
		0b01110,
		0b00100,
		0b00000,
		0b00000
};

uint8_t Cyrilic[8] = {0x1F, 0x10, 0x1E, 0x11, 0x11, 0x11, 0x1E, 0x00}; //E

uint8_t Flecha[8] = {
		0b00000,
		0b00100,
		0b00110,
		0b11111,
		0b00110,
		0b00100,
		0b00000,
		0b00000
};

uint8_t Campana[8] = {
		0b00100,
		0b01110,
		0b01110,
		0b01110,
		0b11111,
		0b00000,
		0b00100,
		0b00000
};

uint8_t degrees[8] = {
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
// FUNCIONES PRIVADAS
// ============================================================================

static void DelayUS(uint32_t us)
{
	uint32_t cycles = (SystemCoreClock/1000000L)*us;
	uint32_t start = DWT->CYCCNT;
	volatile uint32_t cnt;

	do
	{
		cnt = DWT->CYCCNT - start;
	} while(cnt < cycles);
}

static void ExpanderWrite(uint8_t _data)
{
	uint8_t data = _data | dpBacklight;
	HAL_I2C_Master_Transmit(HD44780_hi2c, HD44780_ADDRESS, (uint8_t*)&data, 1, 10);
}

static void PulseEnable(uint8_t _data)
{
	ExpanderWrite(_data | ENABLE);
	DelayUS(20);

	ExpanderWrite(_data & ~ENABLE);
	DelayUS(20);
}

static void Write4Bits(uint8_t value)
{
	ExpanderWrite(value);
	PulseEnable(value);
}

static void Send(uint8_t value, uint8_t mode)
{
	uint8_t highnib = value & 0xF0;
	uint8_t lownib = (value<<4) & 0xF0;
	Write4Bits((highnib)|mode);
	Write4Bits((lownib)|mode);
}

static void SendCommand(uint8_t cmd)
{
	Send(cmd, 0);
}

static void SendChar(uint8_t ch)
{
	Send(ch, RS);
}

static void DelayInit(void)
{
	CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
	CoreDebug->DEMCR |=  CoreDebug_DEMCR_TRCENA_Msk;

	DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; //~0x00000001;
	DWT->CTRL |=  DWT_CTRL_CYCCNTENA_Msk; //0x00000001;

	DWT->CYCCNT = 0;

	/* 3 NO OPERATION instructions */
	__ASM volatile ("NOP");
	__ASM volatile ("NOP");
	__ASM volatile ("NOP");
}

// ============================================================================
// FUNCIONES PÚBLICAS
// ============================================================================

/**
 * @brief Inicializa el módulo HD44780 con el handle I2C.
 *
 * @param hi2c Puntero al handle de I2C utilizado para comunicarse con el HD44780.
 * @param rows
 * @return HD44780_Status_t Estado de la inicialización (OK, ERROR, etc.)
 */
HD44780_Status_t HD44780_Init(I2C_HandleTypeDef* hi2c, uint8_t rows)
{
	if(hi2c == NULL)
	{
		return HD44780_ERROR;
	}

	if(rows == 0)
	{
		return HD44780_ERROR;
	}

	HD44780_hi2c = hi2c;

	dpRows = rows;

	dpBacklight = LCD_BACKLIGHT;

	dpFunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

	if (dpRows > 1)
	{
		dpFunction |= LCD_2LINE;
	}
	else
	{
		dpFunction |= LCD_5x10DOTS;
	}

	/* Wait for initialization */
	DelayInit();
	HAL_Delay(50);

	ExpanderWrite(dpBacklight);
	HAL_Delay(1000);

	/* 4bit Mode */
	Write4Bits(0x03 << 4);
	DelayUS(4500);

	Write4Bits(0x03 << 4);
	DelayUS(4500);

	Write4Bits(0x03 << 4);
	DelayUS(4500);

	Write4Bits(0x02 << 4);
	DelayUS(100);

	/* Display Control */
	SendCommand(LCD_FUNCTIONSET | dpFunction);

	dpControl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	HD44780_Display();
	HD44780_Clear();

	/* Display Mode */
	dpMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	SendCommand(LCD_ENTRYMODESET | dpMode);
	DelayUS(4500);

	HD44780_CreateSpecialChar(0, special1);
	HD44780_CreateSpecialChar(1, special2);
	HD44780_CreateSpecialChar(2, heart);
	HD44780_CreateSpecialChar(3, Cyrilic);
	HD44780_CreateSpecialChar(4, Flecha);
	HD44780_CreateSpecialChar(5, Campana);
	HD44780_CreateSpecialChar(6, degrees);

	HD44780_Home();

	HD44780_Initialized = 1;

	return HD44780_OK;
}

HD44780_Status_t HD44780_Clear()
{
	SendCommand(LCD_CLEARDISPLAY);
	DelayUS(2000);

	return HD44780_OK;
}

HD44780_Status_t HD44780_Home()
{
	SendCommand(LCD_RETURNHOME);
	DelayUS(2000);

	return HD44780_OK;
}

HD44780_Status_t HD44780_SetCursor(uint8_t col, uint8_t row)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if (row >= dpRows)
	{
		row = dpRows-1;
	}
	SendCommand(LCD_SETDDRAMADDR | (col + row_offsets[row]));

	return HD44780_OK;
}

HD44780_Status_t HD44780_NoDisplay()
{
	dpControl &= ~LCD_DISPLAYON;
	SendCommand(LCD_DISPLAYCONTROL | dpControl);

	return HD44780_OK;
}

HD44780_Status_t HD44780_Display()
{
	dpControl |= LCD_DISPLAYON;
	SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_NoCursor()
{
	dpControl &= ~LCD_CURSORON;
	SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_Cursor()
{
	dpControl |= LCD_CURSORON;
	SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_NoBlink()
{
	dpControl &= ~LCD_BLINKON;
	SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_Blink()
{
	dpControl |= LCD_BLINKON;
	SendCommand(LCD_DISPLAYCONTROL | dpControl);
}

HD44780_Status_t HD44780_ScrollDisplayLeft()
{
	SendCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

HD44780_Status_t HD44780_ScrollDisplayRight()
{
	SendCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

HD44780_Status_t HD44780_LeftToRight()
{
	dpMode |= LCD_ENTRYLEFT;
	SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_RightToLeft()
{
	dpMode &= ~LCD_ENTRYLEFT;
	SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_AutoScroll()
{
	dpMode |= LCD_ENTRYSHIFTINCREMENT;
	SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_NoAutoScroll()
{
	dpMode &= ~LCD_ENTRYSHIFTINCREMENT;
	SendCommand(LCD_ENTRYMODESET | dpMode);
}

HD44780_Status_t HD44780_CreateSpecialChar(uint8_t location, uint8_t charmap[])
{
	location &= 0x7;
	SendCommand(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++)
	{
		SendChar(charmap[i]);
	}
}

HD44780_Status_t HD44780_PrintSpecialChar(uint8_t index)
{
	SendChar(index);
}

HD44780_Status_t HD44780_LoadCustomCharacter(uint8_t char_num, uint8_t *rows)
{
	HD44780_CreateSpecialChar(char_num, rows);
}

HD44780_Status_t HD44780_PrintStr(const char c[])
{
	while(*c) SendChar(*c++);
}

HD44780_Status_t HD44780_SetBacklight(uint8_t new_val)
{
	if(new_val) HD44780_Backlight();
	else HD44780_NoBacklight();
}

HD44780_Status_t HD44780_NoBacklight()
{
	dpBacklight=LCD_NOBACKLIGHT;
	ExpanderWrite(0);
}

HD44780_Status_t HD44780_Backlight()
{
	dpBacklight=LCD_BACKLIGHT;
	ExpanderWrite(0);
}
