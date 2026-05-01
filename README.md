# Librería para el módulo LCD HD44780 en STM32

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![STM32](https://img.shields.io/badge/Platform-STM32-black)](https://www.st.com/en/microcontrollers-microprocessors/stm32f4-series.html)
[![Version](https://img.shields.io/badge/Version-2.0.0-green.svg)](https://github.com/Darcko123/HD44780_LCD)
[![Protocol](https://img.shields.io/badge/Protocol-I2C-green.svg)](https://github.com/Darcko123/HD44780_LCD)

> **Fork de:** [eziya/STM32_HAL_I2C_HD44780](https://github.com/eziya/STM32_HAL_I2C_HD44780)  
> Esta librería es un fork del trabajo original de **Eziya**, al cual se le añadió manejo robusto de errores mediante códigos de retorno en todas las funciones públicas, delays basados en DWT y caracteres especiales predefinidos.

---

## Tabla de Contenidos
- [Librería para el módulo LCD HD44780 en STM32](#librería-para-el-módulo-lcd-hd44780-en-stm32)
  - [Tabla de Contenidos](#tabla-de-contenidos)
  - [Descripción](#descripción)
  - [Características](#características)
  - [Pinout y Conexiones](#pinout-y-conexiones)
    - [Pines requeridos](#pines-requeridos)
  - [Configuración I2C](#configuración-i2c)
  - [Instalación](#instalación)
  - [Uso Básico](#uso-básico)
    - [1. Inicialización](#1-inicialización)
    - [2. Imprimir texto](#2-imprimir-texto)
    - [3. Caracteres especiales](#3-caracteres-especiales)
  - [API Reference](#api-reference)
    - [1. Tipos de Datos](#1-tipos-de-datos)
      - [`HD44780_Status_t` - Estados de Retorno](#hd44780_status_t---estados-de-retorno)
    - [2. Funciones Públicas](#2-funciones-públicas)
      - [`HD44780_Init()` - Inicialización del Driver](#hd44780_init---inicialización-del-driver)
      - [`HD44780_Clear()` - Borrar pantalla](#hd44780_clear---borrar-pantalla)
      - [`HD44780_Home()` - Regresar al inicio](#hd44780_home---regresar-al-inicio)
      - [`HD44780_SetCursor()` - Posicionar cursor](#hd44780_setcursor---posicionar-cursor)
      - [`HD44780_PrintStr()` - Imprimir cadena](#hd44780_printstr---imprimir-cadena)
      - [`HD44780_PrintSpecialChar()` - Imprimir carácter especial](#hd44780_printspecialchar---imprimir-carácter-especial)
      - [`HD44780_CreateSpecialChar()` - Crear carácter personalizado](#hd44780_createspecialchar---crear-carácter-personalizado)
      - [`HD44780_LoadCustomCharacter()` - Cargar carácter personalizado](#hd44780_loadcustomcharacter---cargar-carácter-personalizado)
      - [Control de Display](#control-de-display)
      - [Control de Cursor y Parpadeo](#control-de-cursor-y-parpadeo)
      - [Control de Retroiluminación](#control-de-retroiluminación)
      - [Control de Desplazamiento](#control-de-desplazamiento)
      - [Dirección de Escritura](#dirección-de-escritura)
  - [Licencia](#licencia)
  - [Changelog](#changelog)
    - [\[2.0.0\] - 01-05-2026](#200---01-05-2026)
      - [Added](#added)
      - [Changed](#changed)
    - [\[1.x.x\] - Versión Original](#1xx---versión-original)

---

## Descripción

Librería desarrollada en C para la interfaz con el módulo **LCD HD44780** utilizando microcontroladores STM32. Proporciona funciones para controlar el display a través de un expansor I/O **PCF8574** por comunicación I2C, operando en modo de 4 bits. La librería está diseñada para ser fácil de usar, eficiente y compatible con la mayoría de las series STM32 (F1, F4, etc.) utilizando HAL.

Es adecuada para mostrar texto, valores de sensores, menús simples o cualquier información alfanumérica en aplicaciones embebidas.

---

## Características

- **Comunicación I2C**: Interfaz mediante expansor PCF8574 en modo 4 bits, reduciendo los pines GPIO necesarios a solo SDA y SCL.
- **Manejo robusto de errores**: Todas las funciones públicas retornan `HD44780_Status_t`, con códigos específicos para error de I2C, timeout, módulo no inicializado o parámetro inválido.
- **Delays de alta precisión basados en DWT**: Utiliza el contador de ciclos del núcleo Cortex-M para delays en microsegundos sin bloquear el scheduler.
- **Caracteres especiales predefinidos**: Se cargan automáticamente en la CGRAM durante la inicialización (corazón, flecha, campana, símbolo de grados, entre otros).
- **Control completo de display**: Funciones para encender/apagar pantalla, cursor, parpadeo, retroiluminación y modo de desplazamiento.
- **Portabilidad**: Compatible con múltiples familias STM32 mediante la capa HAL. Solo requiere cambiar el `#include` del encabezado HAL en `HD44780.h`.

---

## Pinout y Conexiones

### Pines requeridos

| Pin HD44780 (PCF8574) | Dirección | Descripción           | Tipo GPIO         | Observaciones                          |
|-----------------------|-----------|-----------------------|-------------------|----------------------------------------|
| **VCC**               | Alimentación | 5 V                | N/A               | El módulo PCF8574 opera a 5 V          |
| **GND**               | Tierra    | —                     | N/A               | —                                      |
| **SDA**               | Open-drain | Datos I2C            | `GPIO_AF_OD`      | Requiere resistencia pull-up (4.7 kΩ) |
| **SCL**               | Open-drain | Reloj I2C            | `GPIO_AF_OD`      | Requiere resistencia pull-up (4.7 kΩ) |

> [!NOTE]
> Los módulos comerciales con PCF8574 generalmente incluyen resistencias pull-up integradas en la placa. Verifica si tu módulo ya las trae antes de agregar resistencias externas.

> [!WARNING]
> El LCD HD44780 opera a 5 V. Si tu STM32 trabaja a 3.3 V, verifica que los pines I2C sean tolerantes a 5 V o utiliza un convertidor de nivel lógico.

---

## Configuración I2C

Configura tu periférico I2C en CubeMX/STM32CubeIDE:

| Parámetro           | Valor              | Notas                                        |
|---------------------|--------------------|----------------------------------------------|
| **Mode**            | I2C Master         | El STM32 actúa como maestro                  |
| **Speed Mode**      | Standard (100 kHz) | Fast Mode (400 kHz) también es compatible    |
| **Clock Speed**     | 100000 Hz          | Reducir si hay problemas de comunicación     |
| **Address Length**  | 7 bits             | —                                            |

> [!NOTE]
> La dirección I2C del módulo es: **0x27** (7 bits). La librería la define internamente como `HD44780_ADDRESS (0x27 << 1)` en formato de 8 bits para HAL. Si tu módulo PCF8574 tiene una dirección diferente (p. ej. `0x3F`), ajusta la macro en `HD44780.h`.

---

## Instalación

1. Copia `HD44780.c` y `HD44780.h` a tu proyecto (ej: `Drivers/HD44780/`).
2. Ajusta el `#include` del encabezado HAL en `HD44780.h` según tu familia STM32:
   ```c
   // STM32F0xx:
   #include "stm32f0xx_hal.h"
   // STM32F1xx:
   #include "stm32f1xx_hal.h"
   // STM32F4xx:
   #include "stm32f4xx_hal.h"
   ```
3. Incluye la librería en tu `main.c` o archivo principal:
   ```c
   #include "HD44780.h"
   ```
4. Configura I2C y GPIOs en CubeMX (ver sección anterior).
5. Genera código y compila.

---

## Uso Básico

### 1. Inicialización

```c
// En main() después de HAL_Init() y MX_I2C1_Init()
HD44780_Status_t status = HD44780_Init(&hi2c1, 2);  // display de 2 filas

if (status != HD44780_OK) {
    Error_Handler();  // Módulo no responde o handle NULL
}
```

### 2. Imprimir texto

```c
// Posicionar el cursor en columna 0, fila 0 e imprimir
HD44780_SetCursor(0, 0);
HD44780_PrintStr("Hola STM32!");

// Segunda línea
HD44780_SetCursor(0, 1);
HD44780_PrintStr("Temp: 25.3 C");
```

### 3. Caracteres especiales

```c
// La inicialización ya carga 7 caracteres en posiciones 0-6.
// Índice 6 = símbolo de grados (°)
HD44780_SetCursor(10, 1);
HD44780_PrintStr("25.3");
HD44780_PrintSpecialChar(6);   // imprime °
HD44780_PrintStr("C");

// También puedes definir tu propio carácter en cualquier posición libre (0-7):
uint8_t myChar[8] = {0x0E, 0x0E, 0x04, 0x1F, 0x04, 0x04, 0x04, 0x00};
HD44780_CreateSpecialChar(7, myChar);
HD44780_PrintSpecialChar(7);
```

> [!NOTE]
> La CGRAM del HD44780 permite almacenar hasta **8 caracteres personalizados** (posiciones 0–7). Durante `HD44780_Init()` se cargan 7 de forma automática; la posición 7 queda libre para uso del usuario.

---

## API Reference

### 1. Tipos de Datos

#### `HD44780_Status_t` - Estados de Retorno

Enumeración que define todos los códigos de retorno posibles para las funciones de la librería, permitiendo una gestión robusta de errores y estados del módulo.

```c
typedef enum {
    HD44780_OK              = 0,    /**< Operación exitosa */
    HD44780_ERROR           = 1,    /**< Error en la operación */
    HD44780_TIMEOUT         = 2,    /**< Timeout en la operación */
    HD44780_NOT_INITIALIZED = 3,    /**< Módulo no inicializado */
    HD44780_INVALID_PARAM   = 4,    /**< Parámetro inválido */
} HD44780_Status_t;
```

| Valor                      | Código | Significado                                                         |
|---------------------------|--------|---------------------------------------------------------------------|
| `HD44780_OK`              | 0      | Operación completada sin errores                                    |
| `HD44780_ERROR`           | 1      | Error de I2C, parámetro inválido o condición inesperada             |
| `HD44780_TIMEOUT`         | 2      | Timeout de HAL durante la transmisión I2C                           |
| `HD44780_NOT_INITIALIZED` | 3      | `HD44780_Init()` no se llamó o falló                                |
| `HD44780_INVALID_PARAM`   | 4      | Puntero NULL u otro parámetro fuera de rango                        |

---

### 2. Funciones Públicas

#### `HD44780_Init()` - Inicialización del Driver

Inicializa el módulo HD44780: configura el handle I2C, ejecuta la secuencia de arranque en modo 4 bits, establece el display en estado por defecto (encendido, sin cursor, sin parpadeo) y carga los caracteres especiales predefinidos en la CGRAM.

```c
HD44780_Status_t HD44780_Init(I2C_HandleTypeDef* hi2c, uint8_t rows);
```

| Parámetro | Tipo                   | Descripción                          |
|-----------|------------------------|--------------------------------------|
| `hi2c`    | `I2C_HandleTypeDef*`   | Puntero al handle I2C de HAL         |
| `rows`    | `uint8_t`              | Número de filas del display (1 o 2)  |

**Retorna**: `HD44780_OK` si la inicialización fue exitosa, `HD44780_INVALID_PARAM` si `hi2c` es NULL o `rows` es 0, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

**Secuencia interna:**

1. Valida parámetros y guarda el handle I2C.
2. Inicializa el contador DWT para delays en microsegundos.
3. Ejecuta la secuencia de reset en modo 4 bits según el datasheet del HD44780.
4. Configura el display: modo 4 bits, número de líneas, cursor apagado, retroiluminación encendida.
5. Carga 7 caracteres especiales predefinidos en la CGRAM (posiciones 0–6).

---

#### `HD44780_Clear()` - Borrar pantalla

Borra todo el contenido del display y retorna el cursor a la posición (0, 0).

```c
HD44780_Status_t HD44780_Clear(void);
```

**Retorna**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### `HD44780_Home()` - Regresar al inicio

Retorna el cursor a la posición (0, 0) sin borrar el contenido del display.

```c
HD44780_Status_t HD44780_Home(void);
```

**Retorna**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### `HD44780_SetCursor()` - Posicionar cursor

Mueve el cursor a la columna y fila indicadas.

```c
HD44780_Status_t HD44780_SetCursor(uint8_t col, uint8_t row);
```

| Parámetro | Tipo      | Descripción                              | Rango           |
|-----------|-----------|------------------------------------------|-----------------|
| `col`     | `uint8_t` | Columna destino (0-indexed)              | 0 – 15 (16 col) |
| `row`     | `uint8_t` | Fila destino (0-indexed)                 | 0 – `rows-1`    |

**Retorna**: `HD44780_OK` si la operación fue exitosa, `HD44780_NOT_INITIALIZED` si el módulo no fue inicializado, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### `HD44780_PrintStr()` - Imprimir cadena

Imprime una cadena de caracteres terminada en `\0` en la posición actual del cursor.

```c
HD44780_Status_t HD44780_PrintStr(const char c[]);
```

| Parámetro | Tipo          | Descripción                       |
|-----------|---------------|-----------------------------------|
| `c`       | `const char*` | Cadena de caracteres a imprimir   |

**Retorna**: `HD44780_OK` si la cadena se imprimió completa, `HD44780_INVALID_PARAM` si `c` es NULL, `HD44780_NOT_INITIALIZED` si el módulo no fue inicializado, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### `HD44780_PrintSpecialChar()` - Imprimir carácter especial

Imprime en la posición actual del cursor el carácter almacenado en la CGRAM en el índice indicado.

```c
HD44780_Status_t HD44780_PrintSpecialChar(uint8_t index);
```

| Parámetro | Tipo      | Descripción                              | Rango |
|-----------|-----------|------------------------------------------|-------|
| `index`   | `uint8_t` | Índice del carácter en CGRAM             | 0 – 7 |

**Caracteres predefinidos cargados en `HD44780_Init()`:**

| Índice | Carácter     |
|--------|--------------|
| 0      | Special 1    |
| 1      | Special 2    |
| 2      | Corazón ♥    |
| 3      | Cirílico     |
| 4      | Flecha →     |
| 5      | Campana 🔔   |
| 6      | Grados °     |

**Retorna**: `HD44780_OK` si la operación fue exitosa, `HD44780_NOT_INITIALIZED` si el módulo no fue inicializado, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### `HD44780_CreateSpecialChar()` - Crear carácter personalizado

Define un carácter personalizado de 5×8 píxeles y lo almacena en la CGRAM del LCD en la posición indicada.

```c
HD44780_Status_t HD44780_CreateSpecialChar(uint8_t location, uint8_t charmap[]);
```

| Parámetro  | Tipo       | Descripción                                      | Rango |
|------------|------------|--------------------------------------------------|-------|
| `location` | `uint8_t`  | Posición en CGRAM                                | 0 – 7 |
| `charmap`  | `uint8_t*` | Array de 8 bytes con el mapa de bits del carácter | —    |

**Retorna**: `HD44780_OK` si el carácter se almacenó correctamente, `HD44780_INVALID_PARAM` si `charmap` es NULL, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### `HD44780_LoadCustomCharacter()` - Cargar carácter personalizado

Alias de `HD44780_CreateSpecialChar()`. Carga un carácter personalizado en la CGRAM.

```c
HD44780_Status_t HD44780_LoadCustomCharacter(uint8_t char_num, uint8_t *rows);
```

| Parámetro  | Tipo       | Descripción                                      |
|------------|------------|--------------------------------------------------|
| `char_num` | `uint8_t`  | Posición en CGRAM (0–7)                          |
| `rows`     | `uint8_t*` | Array de 8 bytes con el mapa de bits del carácter |

**Retorna**: `HD44780_OK` si la operación fue exitosa, `HD44780_INVALID_PARAM` si `rows` es NULL, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### Control de Display

| Función                   | Descripción                            |
|---------------------------|----------------------------------------|
| `HD44780_Display(void)`   | Enciende el display                    |
| `HD44780_NoDisplay(void)` | Apaga el display (no borra contenido)  |

```c
HD44780_Status_t HD44780_Display(void);
HD44780_Status_t HD44780_NoDisplay(void);
```

**Retornan**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### Control de Cursor y Parpadeo

| Función                  | Descripción                   |
|--------------------------|-------------------------------|
| `HD44780_Cursor(void)`   | Muestra el cursor (subrayado) |
| `HD44780_NoCursor(void)` | Oculta el cursor              |
| `HD44780_Blink(void)`    | Activa el parpadeo del cursor |
| `HD44780_NoBlink(void)`  | Desactiva el parpadeo         |

```c
HD44780_Status_t HD44780_Cursor(void);
HD44780_Status_t HD44780_NoCursor(void);
HD44780_Status_t HD44780_Blink(void);
HD44780_Status_t HD44780_NoBlink(void);
```

**Retornan**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### Control de Retroiluminación

| Función                              | Descripción                              |
|--------------------------------------|------------------------------------------|
| `HD44780_Backlight(void)`            | Enciende la retroiluminación             |
| `HD44780_NoBacklight(void)`          | Apaga la retroiluminación                |
| `HD44780_SetBacklight(uint8_t val)`  | Enciende si `val != 0`, apaga si `val == 0` |

```c
HD44780_Status_t HD44780_Backlight(void);
HD44780_Status_t HD44780_NoBacklight(void);
HD44780_Status_t HD44780_SetBacklight(uint8_t new_val);
```

**Retornan**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### Control de Desplazamiento

| Función                           | Descripción                              |
|-----------------------------------|------------------------------------------|
| `HD44780_ScrollDisplayLeft(void)` | Desplaza el contenido del display a la izquierda |
| `HD44780_ScrollDisplayRight(void)`| Desplaza el contenido del display a la derecha   |
| `HD44780_AutoScroll(void)`        | Activa el desplazamiento automático al escribir  |
| `HD44780_NoAutoScroll(void)`      | Desactiva el desplazamiento automático           |

```c
HD44780_Status_t HD44780_ScrollDisplayLeft(void);
HD44780_Status_t HD44780_ScrollDisplayRight(void);
HD44780_Status_t HD44780_AutoScroll(void);
HD44780_Status_t HD44780_NoAutoScroll(void);
```

**Retornan**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

#### Dirección de Escritura

| Función                     | Descripción                                 |
|-----------------------------|---------------------------------------------|
| `HD44780_LeftToRight(void)` | El cursor avanza de izquierda a derecha (por defecto) |
| `HD44780_RightToLeft(void)` | El cursor avanza de derecha a izquierda     |

```c
HD44780_Status_t HD44780_LeftToRight(void);
HD44780_Status_t HD44780_RightToLeft(void);
```

**Retornan**: `HD44780_OK` si la operación fue exitosa, `HD44780_ERROR` / `HD44780_TIMEOUT` si falla la comunicación I2C.

---

## Licencia

Este proyecto está bajo la licencia MIT. Consulta el archivo [LICENSE](LICENSE) para más detalles.

---

## Changelog

Todos los cambios notables de esta librería se documentan en esta sección.  
El formato está basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/).

---

### [2.0.0] - 01-05-2026

#### Added

- Tipo de retorno `HD44780_Status_t` con cinco códigos: `OK`, `ERROR`, `TIMEOUT`, `NOT_INITIALIZED`, `INVALID_PARAM`.
- Todas las funciones públicas retornan `HD44780_Status_t` y propagan errores I2C correctamente.
- Delays de microsegundos basados en el contador DWT del núcleo Cortex-M (`DelayUS`, `DelayInit`).
- Siete caracteres especiales predefinidos (corazón, flecha, campana, símbolo de grados, etc.) cargados automáticamente en `HD44780_Init()`.
- Validación de parámetros en `HD44780_Init()`, `HD44780_CreateSpecialChar()`, `HD44780_PrintStr()` y `HD44780_LoadCustomCharacter()`.
- Parámetro `rows` en `HD44780_Init()` para soporte de displays de 1 y 2 líneas.

#### Changed

- Refactorización completa del código fuente siguiendo convenciones de librería STM32 HAL.
- Las funciones internas (`ExpanderWrite`, `PulseEnable`, `Write4Bits`, `Send`, `SendCommand`, `SendChar`) retornan y propagan `HD44780_Status_t`.

---

### [1.x.x] - Versión Original

Desarrollada por **Eziya**. Repositorio original: [eziya/STM32_HAL_I2C_HD44780](https://github.com/eziya/STM32_HAL_I2C_HD44780).

Esta versión implementó la interfaz básica del HD44780 con expansor PCF8574 sobre I2C para STM32 HAL, basándose en la librería Arduino [LiquidCrystal_I2C](https://github.com/johnrickman/LiquidCrystal_I2C).
