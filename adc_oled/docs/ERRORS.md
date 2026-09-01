# ERRORS.md - Reporte de Errores de Implementación

Análisis detallado de bugs, problemas y errores encontrados en el código fuente del proyecto **adc_oled**.

---

## Resumen

| Severidad | Cantidad | Estado |
|-----------|----------|--------|
| 🔴 Crítico | 2 | Activo |
| 🟠 Mayor | 4 | Activo |
| 🟡 Menor | 5 | Activo |
| ⚪ Cosmético | 3 | Activo |
| **Total** | **14** | **0 corregidos** |

---

## 🔴 ERRORES CRÍTICOS

### ERROR-001: Overflow en `adc_sum` (uint16_t)

**Archivo:** `src/main.c`, líneas 60, 520
**Severidad:** 🔴 Crítico — Rompe el cálculo del voltaje promedio

**Descripción:**
La variable `adc_sum` es de tipo `uint16_t` (rango 0–65535), pero se acumulan las muestras ADC sin verificación de overflow.

**Código afectado:**
```c
static uint16_t adc_sum = 0;      // ← Línea 60: uint16_t, max 65535
static uint16_t adc_samples = 0;  // ← Línea 61

// En adc_sample():
adc_sum += raw;    // ← Línea ~470: puede overflow
adc_samples++;     // ← Línea ~471

// En display update:
adc_average = (uint16_t)(adc_sum / adc_samples);  // ← Línea ~520: resultado corrupto
```

**Escenario de fallo:**
- Con `scope_window_ms = 500` y `DISPLAY_UPDATE_MS = 200`
- En 200ms a 250Hz → ~50 muestras
- Con señal de ~2000 ADC: 50 × 2000 = **100,000** > 65,535
- Resultado: `adc_sum` se desborda, `adc_average` muestra valor incorrecto
- El display muestra voltaje erróneo

**Solución:**
```c
static uint32_t adc_sum = 0;  // Cambiar a uint32_t
```

---

### ERROR-002: `malloc` sin NULL check en `SSD1306_send_buf`

**Archivo:** `src/main.c`, línea ~170
**Severidad:** 🔴 Crítico — Puede causar crash del MCU

**Descripción:**
La función `SSD1306_send_buf` llama a `malloc(buflen + 1)` sin verificar si retorna NULL. En un microcontrolador con 264KB SRAM, es posible que `malloc` falle, especialmente después de uso prolongado.

**Código afectado:**
```c
void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t *temp_buf = malloc(buflen + 1);  // ← Puede retornar NULL
    temp_buf[0] = 0x40;                       // ← CRASH si NULL
    memcpy(temp_buf + 1, buf, buflen);        // ← CRASH si NULL
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);
    free(temp_buf);
}
```

**Impacto:**
- Si `malloc` falla, el MCU se cuelga o resetea
- Ocurriría al enviar cada frame del display (cada 200ms)
- El dispositivo quedaría inutilizable

**Solución:**
```c
void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t *temp_buf = malloc(buflen + 1);
    if (!temp_buf) return;  // ← Agregar check
    temp_buf[0] = 0x40;
    memcpy(temp_buf + 1, buf, buflen);
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, temp_buf, buflen + 1, false);
    free(temp_buf);
}
```

**Alternativa mejor (sin malloc):**
```c
void SSD1306_send_buf(uint8_t buf[], int buflen) {
    uint8_t header = 0x40;
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, &header, 1, true);  // restart
    i2c_write_blocking(I2C_PORT, SSD1306_I2C_ADDR, buf, buflen, false);
}
```

---

## 🟠 ERRORES MAYORES

### ERROR-003: Macros `SSD1306_SET_COM_OUT_DIR` y `_FLIP` idénticas

**Archivo:** `src/main.c`, líneas 83–84
**Severidad:** 🟠 Mayor — Confusión de código, posible error lógico

**Descripción:**
Las macros `SSD1306_SET_COM_OUT_DIR` y `SSD1306_SET_COM_OUT_DIR_FLIP` están definidas con el mismo valor `0xC0`, pero deberían ser diferentes.

**Código afectado:**
```c
#define SSD1306_SET_COM_OUT_DIR     _u(0xC0)  // ← Línea 83
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC0) // ← Línea 84: ¡IDÉNTICA!
```

**Problema:**
- `SSD1306_SET_COM_OUT_DIR` = 0xC0 (scan normal: COM0→COM63)
- `SSD1306_SET_COM_OUT_DIR_FLIP` debería ser 0xC8 (scan invertido: COM63→COM0)
- Ambas valen 0xC0, la flip no tiene efecto

**Solución:**
```c
#define SSD1306_SET_COM_OUT_DIR      _u(0xC0)
#define SSD1306_SET_COM_OUT_DIR_FLIP _u(0xC8)  // ← Corregir
```

---

### ERROR-004: `trigger_scale` declarada pero nunca usada en render

**Archivo:** `src/main.c`, líneas 55, 360
**Severidad:** 🟠 Mayor — Feature incompleto

**Descripción:**
La variable `trigger_scale` se puede configurar por serial (`SCALE:50`) pero nunca se utiliza en el cálculo del waveform.

**Código afectado:**
```c
static float trigger_scale = 10.0f;  // ← Línea 55: se declara

// En process_serial_command():
} else if (strncmp(cmd, "SCALE:", 6) == 0) {
    float scale = atof(cmd + 6);
    if (scale >= 0.1f && scale <= 100.0f) {
        trigger_scale = scale;  // ← Línea 360: se actualiza
        // ...
    }
}

// En display update: ¡trigger_scale no se usa!
uint8_t y0 = trace_y + trace_h - ((val0 * trace_h) / 4095);  // ← Sin escala
```

**Impacto:**
- El usuario puede cambiar `SCALE` pero no ve efecto
- Confusión: la feature parece rota

**Solución:**
Implementar zoom: `val = (val * trigger_scale) / 100.0` o documentar como "próximamente".

---

### ERROR-005: Comentario incorrecto "512ms" en header

**Archivo:** `src/main.c`, línea 7
**Severidad:** 🟠 Mayor — Documentación incorrecta

**Descripción:**
El comentario del archivo dice "512ms de ventana" pero el valor real es 500ms.

**Código afectado:**
```c
 * - Abajo: trazo tipo osciloscopio, 512ms de ventana  // ← INCORRECTO
```

**Solución:**
```c
 * - Abajo: trazo tipo osciloscopio, 500ms de ventana
```

---

### ERROR-006: `cmd_buf` estática sin limpieza en reconexión USB

**Archivo:** `src/main.c`, líneas 334–335
**Severidad:** 🟠 Mayor — Datos parciales en reconexión

**Descripción:**
El buffer de comandos `cmd_buf` es estático y no se limpia cuando USB se reconecta. Si se envían datos parciales antes de la reconexión, el buffer retaina datos obsoletos.

**Código afectado:**
```c
static void check_serial_commands(void) {
    if (!tud_cdc_connected()) return;

    static char cmd_buf[64];     // ← Nunca se limpia
    static uint8_t cmd_len = 0;  // ← Nunca se resetea

    while (tud_cdc_available()) {
        // ...
    }
}
```

**Escenario:**
1. Usuario envía `TRIG:1.` (incompleto)
2. USB se reconecta
3. Siguiente comando `500\r\n` se concatena: `TRIG:1.500`
4. Resultado inesperado

**Solución:**
```c
void tud_mount_cb(void) {
    // Reset command buffer on USB connect
    // Necesita acceso a cmd_len estático
    cdc_send_string("[USB] Device mounted\r\n");
}
```

---

## 🟡 ERRORES MENORES

### ERROR-007: `atof()` sin validación de input

**Archivo:** `src/main.c`, línea 316
**Severidad:** 🟡 Menor — Input inválido se acepta silenciosamente

**Descripción:**
`atof()` convierte cualquier string no numérico a `0.0`, que es un valor válido para TRIG. No hay validación de que el input sea realmente numérico.

**Código afectado:**
```c
float voltage = atof(cmd + 5);  // ← "TRIG:abc" → 0.0 (válido)
if (voltage >= 0.0f && voltage <= 3.3f) {
    // Se acepta silenciosamente
}
```

**Solución:**
```c
char *endptr;
float voltage = strtof(cmd + 5, &endptr);
if (endptr == cmd + 5 || *endptr != '\0') {
    cdc_send_string("[CFG] Error: invalid number\r\n");
    return;
}
```

---

### ERROR-008: `noise_floor` incremento extremadamente lento

**Archivo:** `src/main.c`, líneas 537–540
**Severidad:** 🟡 Menor — Funcionalidad limitada

**Descripción:**
El `noise_floor` se incrementa de 1 en 1 por muestreo. A 250Hz, tarda ~17 segundos en subir 100 unidades.

**Código afectado:**
```c
if (current_adc_value < noise_floor) {
    noise_floor = current_adc_value;    // Baja rápido
} else if (noise_floor < trigger_adc_level) {
    noise_floor++;                      // ← Sube 1 por muestreo (~4ms)
}
// Para subir de 0 a 1000: 1000 × 4ms = 4 segundos
// Para subir de 0 a 2000: 2000 × 4ms = 8 segundos
```

**Solución:**
```c
// Incremento proporcional
if (noise_floor < trigger_adc_level) {
    uint16_t diff = trigger_adc_level - noise_floor;
    noise_floor += (diff > 10) ? diff / 10 : 1;
}
```

---

### ERROR-009: `SSD1306_send_buf` malloc/free en cada frame

**Archivo:** `src/main.c`, línea ~170
**Severidad:** 🟡 Menor — Ineficiencia en MCU

**Descripción:**
`SSD1306_send_buf` ejecuta `malloc` y `free` cada vez que se envía un frame al display. Con actualización cada 200ms, esto son ~5 allocs/segundo que fragmentan la memoria.

**Impacto:**
- Fragmentación de memoria a largo plazo
- Overhead de alloc/free innecesario
- Riesgo de OOM si hay otros allocs

**Solución:** Ver ERROR-002 (usar write directo o buffer estático).

---

### ERROR-010: `WriteChar`/`WriteString` con `char *` en lugar de `const char *`

**Archivo:** `src/main.c`, líneas 255, 263
**Severidad:** 🟡 Menor — Warning de compilador, potencial bug con char con signo

**Descripción:**
`WriteChar` recibe `uint8_t ch` pero `WriteString` recibe `char *str` (con signo). `toupper()` con char con signo puede causar problemas con caracteres > 127.

**Código afectado:**
```c
static void WriteString(uint8_t *buf, int16_t x, int16_t y, char *str) {
    //                      ^^^^ char con signo
    while (*str) {
        WriteChar(buf, x, y, *str++);  // ← char → uint8_t (implícito)
        x += 8;
    }
}
```

**Solución:**
```c
static void WriteString(uint8_t *buf, int16_t x, int16_t y, const char *str) {
```

---

## ⚪ ERRORES COSMÉTICOS

### ERROR-011: `usb_descriptors.h` header vacío

**Archivo:** `src/usb_descriptors.h`
**Severidad:** ⚪ Cosmético

**Descripción:**
El header solo contiene include guard, sin prototipos ni definiciones útiles.

**Solución:** Agregar prototipos o eliminar el archivo.

---

### ERROR-012: `nlohmann/` incluido sin usar

**Archivo:** `src/nlohmann/`
**Severidad:** ⚪ Cosmético

**Descripción:**
La librería JSON de nlohmann está en el proyecto pero no se incluye en ningún archivo. Ocupa espacio innecesario.

**Solución:** Usar para export de datos o eliminar la carpeta.

---

### ERROR-013: `SSD1306_scroll()` declarada pero nunca llamada

**Archivo:** `src/main.c`, línea ~210
**Severidad:** ⚪ Cosmético

**Descripción:**
La función `SSD1306_scroll(bool on)` está implementada pero nunca se invoca en el código.

**Solución:** Eliminar si no se planea usar, o documentar como "próximamente".

---

## Tabla de Seguimiento

| ID | Descripción | Severidad | Archivo | Línea | Estado |
|----|-------------|-----------|---------|-------|--------|
| 001 | adc_sum overflow | 🔴 | main.c | 60 | 🔴 Abierto |
| 002 | malloc sin NULL check | 🔴 | main.c | 170 | 🔴 Abierto |
| 003 | Macros COM_OUT_DIR duplicadas | 🟠 | main.c | 83 | 🟠 Abierto |
| 004 | trigger_scale no usado | 🟠 | main.c | 55 | 🟠 Abierto |
| 005 | Comentario 512ms incorrecto | 🟠 | main.c | 7 | 🟠 Abierto |
| 006 | cmd_buf sin limpieza USB | 🟠 | main.c | 334 | 🟠 Abierto |
| 007 | atof sin validación | 🟡 | main.c | 316 | 🟡 Abierto |
| 008 | noise_floor lento | 🟡 | main.c | 537 | 🟡 Abierto |
| 009 | malloc/free cada frame | 🟡 | main.c | 170 | 🟡 Abierto |
| 010 | char * vs const char * | 🟡 | main.c | 263 | 🟡 Abierto |
| 011 | Header USB vacío | ⚪ | usb_descriptors.h | — | ⚪ Abierto |
| 012 | nlohmann sin usar | ⚪ | src/nlohmann/ | — | ⚪ Abierto |
| 013 | SSD1306_scroll sin llamar | ⚪ | main.c | 210 | ⚪ Abierto |

---

## Recomendación de Corrección

```
Orden de prioridad:
1. ERROR-001 (adc_sum) — Crítico, rompe display
2. ERROR-002 (malloc) — Crítico, puede causar crash
3. ERROR-003 (macros) — Mayor, confusión de código
4. ERROR-004 (trigger_scale) — Mayor, feature roto
5. ERROR-007 (atof) — Menor, validación de input
6. ERROR-006 (cmd_buf) — Mayor, edge case USB
7. Resto — Mejoras menores
```

---

## Nota sobre el SDK y TinyUSB

Los errores listados son del código de aplicación (`main.c`). El SDK de Pico y TinyUSB son bibliotecas battle-tested y no se espera que contengan bugs relevantes para este proyecto. Los issues de TinyUSB deberían reportarse upstream.
