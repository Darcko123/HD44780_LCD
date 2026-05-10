/**
 * @file HD44780.c
 * @brief Implementation of the HD44780 LCD library.
 *
 * @author Eziya - Daniel Ruiz
 * @date April 30, 2026
 * @version 2.1.0
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

const uint8_t special1[8] = {
        0b00000,
        0b11001,
        0b11011,
        0b00110,
        0b01100,
        0b11011,
        0b10011,
        0b00000
};

const uint8_t special2[8] = {
        0b11000,
        0b11000,
        0b00110,
        0b01001,
        0b01000,
        0b01001,
        0b00110,
        0b00000
};

const uint8_t heart[8] = {
        0b00000,
        0b01010,
        0b11111,
        0b11111,
        0b01110,
        0b00100,
        0b00000,
        0b00000
};

const uint8_t Cyrillic[8] = {
        0b11111, 
        0b10000, 
        0b11110, 
        0b10001, 
        0b10001, 
        0b10001, 
        0b11110, 
        0b00000
};

const uint8_t Flecha[8] = {
        0b00000,
        0b00100,
        0b00110,
        0b11111,
        0b00110,
        0b00100,
        0b00000,
        0b00000
};

const uint8_t Campana[8] = {
        0b00100,
        0b01110,
        0b01110,
        0b01110,
        0b11111,
        0b00000,
        0b00100,
        0b00000
};

const uint8_t degrees[8] = {
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

/**
 * @brief Genera un retardo en microsegundos usando el contador de ciclos DWT.
 *
 * @param[in] us Tiempo de espera en microsegundos.
 */
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

/**
 * @brief Habilita el contador de ciclos DWT para temporización de microsegundos.
 */
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

/**
 * @brief Envía un byte al expansor I2C PCF8574, incluyendo el bit de retroiluminación.
 *
 * @param[in] _data Dato a enviar al expansor (se aplica OR con dpBacklight).
 * @return HD44780_Status_t HD44780_OK si la transmisión fue exitosa,
 *         HD44780_TIMEOUT o HD44780_ERROR en caso contrario.
 */
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

/**
 * @brief Genera un pulso en el pin Enable para latchear datos en el LCD.
 *
 * @param[in] _data Dato con el bit Enable a pulsar.
 * @return HD44780_Status_t HD44780_OK si el pulso fue exitoso, error en caso contrario.
 */
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

/**
 * @brief Escribe un nibble de 4 bits en el LCD a través del expansor I2C.
 *
 * @param[in] value Nibble a escribir (bits [7:4] contienen los datos).
 * @return HD44780_Status_t HD44780_OK si la escritura fue exitosa, error en caso contrario.
 */
static HD44780_Status_t Write4Bits(uint8_t value)
{
    HD44780_Status_t status;

    status = ExpanderWrite(value);
    if (status != HD44780_OK) { return status; }
    return PulseEnable(value);
}

/**
 * @brief Envía un byte completo al LCD en modo 4 bits (nibble alto primero, luego nibble bajo).
 *
 * @param[in] value Byte a enviar al LCD.
 * @param[in] mode  Modo de envío: 0 para comando (RS=0), RS para dato (RS=1).
 * @return HD44780_Status_t HD44780_OK si el envío fue exitoso, error en caso contrario.
 */
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

/**
 * @brief Envía un byte de comando al LCD (RS = 0).
 *
 * @param[in] cmd Comando a enviar al controlador HD44780.
 * @return HD44780_Status_t HD44780_OK si el comando fue enviado correctamente, error en caso contrario.
 */
static HD44780_Status_t SendCommand(uint8_t cmd)
{
    return Send(cmd, 0);
}

/**
 * @brief Envía un byte de dato (carácter) al LCD (RS = 1).
 *
 * @param[in] ch Carácter a mostrar en el LCD.
 * @return HD44780_Status_t HD44780_OK si el carácter fue enviado correctamente, error en caso contrario.
 */
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
 * @param[in] rows Número de filas del display.
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
    HAL_Delay(20);

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

    status = SendCommand(LCD_RETURNHOME);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);

    HD44780_Initialized = 1U;

    return HD44780_OK;
}

/**
 * @brief Borra el contenido del display y retorna el cursor a la posición inicial (0,0).
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_Clear(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    HD44780_Status_t status = SendCommand(LCD_CLEARDISPLAY);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);
    return HD44780_OK;
}

/**
 * @brief Retorna el cursor a la posición inicial (0,0) sin borrar el contenido del display.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_Home(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    HD44780_Status_t status = SendCommand(LCD_RETURNHOME);
    if (status != HD44780_OK) { return status; }
    DelayUS(2000);
    return HD44780_OK;
}

/**
 * @brief Posiciona el cursor en la columna y fila especificadas.
 *
 * @param[in] col Columna (posición horizontal), comienza en 0.
 * @param[in] row Fila (posición vertical), comienza en 0. Si supera el número de filas
 *                configuradas, se ajusta a la última fila disponible.
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_SetCursor(uint8_t col, uint8_t row)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    const uint8_t row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row >= dpRows) { row = dpRows - 1; }
    return SendCommand(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

/**
 * @brief Apaga el display conservando el contenido en la memoria del LCD.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_NoDisplay(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempControl = dpControl & ~LCD_DISPLAYON;
    HD44780_Status_t status = SendCommand(LCD_DISPLAYCONTROL | tempControl);
    if (status == HD44780_OK) { dpControl = tempControl; }
    return status;
}

/**
 * @brief Enciende el display para mostrar el contenido almacenado en la memoria del LCD.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_Display(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempControl = dpControl | LCD_DISPLAYON;
    HD44780_Status_t status = SendCommand(LCD_DISPLAYCONTROL | tempControl);
    if (status == HD44780_OK) { dpControl = tempControl; }
    return status;
}

/**
 * @brief Oculta el cursor de subrayado del display.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_NoCursor(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempControl = dpControl & ~LCD_CURSORON;
    HD44780_Status_t status = SendCommand(LCD_DISPLAYCONTROL | tempControl);
    if (status == HD44780_OK) { dpControl = tempControl; }
    return status;
}

/**
 * @brief Muestra el cursor de subrayado en la posición actual del display.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_Cursor(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempControl = dpControl | LCD_CURSORON;
    HD44780_Status_t status = SendCommand(LCD_DISPLAYCONTROL | tempControl);
    if (status == HD44780_OK) { dpControl = tempControl; }
    return status;
}

/**
 * @brief Desactiva el parpadeo del cursor.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_NoBlink(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempControl = dpControl & ~LCD_BLINKON;
    HD44780_Status_t status = SendCommand(LCD_DISPLAYCONTROL | tempControl);
    if (status == HD44780_OK) { dpControl = tempControl; }
    return status;
}

/**
 * @brief Activa el parpadeo del cursor (bloque parpadeante en la posición actual).
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_Blink(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempControl = dpControl | LCD_BLINKON;
    HD44780_Status_t status = SendCommand(LCD_DISPLAYCONTROL | tempControl);
    if (status == HD44780_OK) { dpControl = tempControl; }
    return status;
}

/**
 * @brief Desplaza el contenido del display una posición hacia la izquierda.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_ScrollDisplayLeft(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    return SendCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

/**
 * @brief Desplaza el contenido del display una posición hacia la derecha.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_ScrollDisplayRight(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    return SendCommand(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

/**
 * @brief Configura la dirección de escritura de izquierda a derecha (modo por defecto).
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_LeftToRight(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempMode = dpMode | LCD_ENTRYLEFT;
    HD44780_Status_t status = SendCommand(LCD_ENTRYMODESET | tempMode);
    if (status == HD44780_OK) { dpMode = tempMode; }
    return status;
}

/**
 * @brief Configura la dirección de escritura de derecha a izquierda.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_RightToLeft(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempMode = dpMode & ~LCD_ENTRYLEFT;
    HD44780_Status_t status = SendCommand(LCD_ENTRYMODESET | tempMode);
    if (status == HD44780_OK) { dpMode = tempMode; }
    return status;
}

/**
 * @brief Activa el desplazamiento automático del display al escribir un carácter.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_AutoScroll(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t tempMode = dpMode | LCD_ENTRYSHIFTINCREMENT;
    HD44780_Status_t status = SendCommand(LCD_ENTRYMODESET | tempMode);
    if (status == HD44780_OK) { dpMode = tempMode; }
    return status;
}

/**
 * @brief Desactiva el desplazamiento automático del display.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_NoAutoScroll(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }
    
    uint8_t tempMode = dpMode & ~LCD_ENTRYSHIFTINCREMENT;
    HD44780_Status_t status = SendCommand(LCD_ENTRYMODESET | tempMode);
    if (status == HD44780_OK) { dpMode = tempMode; }
    return status;
}

/**
 * @brief Crea un carácter personalizado en la memoria CGRAM del LCD.
 *
 * @param[in] location Posición en CGRAM donde almacenar el carácter (0–7).
 * @param[in] charmap  Array de 8 bytes con el patrón del carácter (5 bits útiles por fila).
 * @return HD44780_Status_t
 *         - HD44780_OK              si el carácter fue creado correctamente.
 *         - HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 *         - HD44780_INVALID_PARAM   si @p charmap es NULL.
 */
HD44780_Status_t HD44780_CreateSpecialChar(uint8_t location, const uint8_t charmap[])
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }
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

/**
 * @brief Imprime en el display un carácter especial previamente guardado en CGRAM.
 *
 * @param[in] index Índice del carácter en CGRAM (0–7), correspondiente al usado en
 *                  HD44780_CreateSpecialChar().
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_PrintSpecialChar(uint8_t index)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    return SendChar(index);
}

/**
 * @brief Imprime una cadena de caracteres terminada en null en el display.
 *
 * @param[in] c Puntero a la cadena de caracteres a imprimir.
 * @return HD44780_Status_t
 *         - HD44780_OK              si todos los caracteres fueron enviados correctamente.
 *         - HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 *         - HD44780_INVALID_PARAM   si @p c es NULL.
 */
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

/**
 * @brief Establece el estado de la retroiluminación del display.
 *
 * @param[in] new_val Valor de retroiluminación: distinto de 0 para encender, 0 para apagar.
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_SetBacklight(uint8_t new_val)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }
    if (new_val) { return HD44780_Backlight();   }
    else         { return HD44780_NoBacklight(); }
}

/**
 * @brief Apaga la retroiluminación del display.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_NoBacklight(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }

    uint8_t prevBacklight = dpBacklight;
    dpBacklight = LCD_NOBACKLIGHT;
    HD44780_Status_t status = ExpanderWrite(0);
    if (status != HD44780_OK) { dpBacklight = prevBacklight; }
    return status;
}

/**
 * @brief Enciende la retroiluminación del display.
 *
 * @return HD44780_Status_t HD44780_OK si la operación fue exitosa,
 *         HD44780_NOT_INITIALIZED si el módulo no fue inicializado.
 */
HD44780_Status_t HD44780_Backlight(void)
{
    if (HD44780_Initialized != 1U) { return HD44780_NOT_INITIALIZED; }
    
    uint8_t prevBacklight = dpBacklight;
    dpBacklight = LCD_BACKLIGHT;
    HD44780_Status_t status = ExpanderWrite(0);
    if (status != HD44780_OK) { dpBacklight = prevBacklight; }
    return status;
}