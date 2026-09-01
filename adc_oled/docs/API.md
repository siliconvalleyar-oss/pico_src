# API.md - Protocolo Serial USB CDC

Documentación completa de la API de comunicación por serial del firmware **adc_oled v1.2.0**.

---

## 1. Conexión

### Puerto Serial

| Sistema | Puerto | Velocidad |
|---------|--------|-----------|
| Linux | `/dev/ttyACM0` | 115200 baud |
| macOS | `/dev/cu.usbmodem*` | 115200 baud |
| Windows | `COMx` (Administrador de dispositivos) | 115200 baud |

### Parámetros

```
Baud rate:    115200
Data bits:    8
Stop bits:    1
Parity:       None
Flow control: None
```

### Conexión rápida (Linux)

```bash
# minicom
minicom -D /dev/ttyACM0 -b 115200

# screen
screen /dev/ttyACM0 115200

# picocom
picocom -b 115200 /dev/ttyACM0

# stty + cat
stty -F /dev/ttyACM0 115200 raw -echo
cat /dev/ttyACM0
```

### Envío de comandos

```bash
# Método 1: echo
echo "TRIG:1.700" > /dev/ttyACM0

# Método 2: printf (con \r\n)
printf "TRIG:1.700\r\n" > /dev/ttyACM0

# Método 3: Python
import serial
pico = serial.Serial('/dev/ttyACM0', 115200)
pico.write(b'TRIG:1.700\r\n')
print(pico.readline().decode())
```

---

## 2. Comandos

### 2.1 `TRIG:voltaje`

Establece el nivel de trigger en voltios.

| Campo | Tipo | Rango | Default |
|-------|------|-------|---------|
| voltaje | float | 0.000 – 3.300 | 1.700 |

**Ejemplo:**
```
> TRIG:1.700
< [CFG] Trigger set to 1.700V (2123 ADC)

> TRIG:0.500
< [CFG] Trigger set to 0.500V (613 ADC)

> TRIG:3.300
< [CFG] Trigger set to 3.300V (4095 ADC)
```

**Error:**
```
> TRIG:5.000
< [CFG] Error: voltage must be 0.000 - 3.300V
```

**Conversión interna:**
```
trigger_adc_level = (trigger_voltage_mv / 1000.0) * 4095.0 / 3.3
```

---

### 2.2 `TRIGMODE:modo`

Configura el modo de detección del trigger.

| Campo | Tipo | Valores | Default |
|-------|------|---------|---------|
| modo | uint8 | 0 o 1 | 0 |

| Valor | Modo | Descripción |
|-------|------|-------------|
| 0 | ACTIVE LOW | Se dispara cuando la señal **BAJA** por debajo del nivel |
| 1 | ACTIVE HIGH | Se dispara cuando la señal **SUBE** por encima del nivel |

**Ejemplo:**
```
> TRIGMODE:0
< [CFG] Trigger mode set to ACTIVE LOW (falling)

> TRIGMODE:1
< [CFG] Trigger mode set to ACTIVE HIGH (rising)
```

**Error:**
```
> TRIGMODE:2
< [CFG] Error: TRIGMODE must be 0 or 1
```

---

### 2.3 `SCALE:valor`

Configura la escala/amplitud del display.

| Campo | Tipo | Rango | Default |
|-------|------|-------|---------|
| valor | float | 0.1 – 100.0 | 10.0 |

**Ejemplo:**
```
> SCALE:50.0
< [CFG] Scale set to 50.0

> SCALE:0.5
< [CFG] Scale set to 0.5
```

**Error:**
```
> SCALE:200
< [CFG] Error: scale must be 0.1 - 100.0
```

**Nota:** Esta variable se almacena pero actualmente no afecta la visualización del waveform. Ver [TODO.md](TODO.md) para implementación pendiente.

---

### 2.4 `TIME:ms`

Configura la ventana de tiempo del osciloscopio.

| Campo | Tipo | Rango | Default |
|-------|------|-------|---------|
| ms | uint16 | 100 – 5000 | 500 |

**Ejemplo:**
```
> TIME:1000
< [CFG] Scope window set to 1000ms (7ms/sample)

> TIME:200
< [CFG] Scope window set to 200ms (1ms/sample)
```

**Cálculo del intervalo de muestreo:**
```
sample_interval_ms = scope_window_ms / 128
if (sample_interval_ms < 1) sample_interval_ms = 1
```

| Ventana | Intervalo | Frecuencia aproximada |
|---------|-----------|----------------------|
| 100 ms | 1 ms | ~1000 Hz |
| 200 ms | 1 ms | ~1000 Hz |
| 500 ms | 4 ms | ~250 Hz |
| 1000 ms | 7 ms | ~143 Hz |
| 2000 ms | 15 ms | ~67 Hz |
| 5000 ms | 39 ms | ~26 Hz |

**Error:**
```
> TIME:50
< [CFG] Error: TIME must be 100-5000 ms
```

---

### 2.5 `MONITOR:ON` / `MONITOR:OFF`

Habilita o deshabilita el envío periódico de datos de debug.

| Campo | Tipo | Valores | Default |
|-------|------|---------|---------|
| estado | string | `ON` o `OFF` | OFF |

**Ejemplo:**
```
> MONITOR:ON
< [CFG] Monitor enabled

> MONITOR:OFF
< [CFG] Monitor disabled
```

**Datos enviados cuando está habilitado (cada 1 segundo):**
```
[DATA] ADC= 1234 | V= 1.234V | TRIG=2123 | DO=HIGH
```

| Campo | Descripción |
|-------|-------------|
| ADC | Valor raw del ADC (0–4095) |
| V | Voltaje calculado (0.000–3.300V) |
| TRIG | Nivel de trigger en counts ADC |
| DO | Estado digital: HIGH o LOW |

**Error:**
```
> MONITOR:QUIET
< [CFG] Error: use MONITOR:ON or MONITOR:OFF
```

---

### 2.6 `GET`

Muestra la configuración actual del dispositivo.

**Ejemplo:**
```
> GET
< [CFG] Trigger=1.700V (2123 ADC) | Mode=LOW | Scale=10.0 | Window=500ms | Monitor=OFF | NoiseFloor=1234
```

| Campo | Descripción |
|-------|-------------|
| Trigger | Nivel de trigger en voltios y counts ADC |
| Mode | Modo de trigger: LOW o HIGH |
| Scale | Escala/amplitud configurada |
| Window | Ventana de tiempo en ms |
| Monitor | Estado del monitor serial |
| NoiseFloor | Piso de ruido actual (se actualiza dinámicamente) |

---

### 2.7 `VERSION`

Retorna la versión del firmware.

**Ejemplo:**
```
> VERSION
< [VERSION] adc_oled v1.2.0
```

---

## 3. Formato de Respuesta

### Prefijos

| Prefijo | Tipo | Descripción |
|---------|------|-------------|
| `[CFG]` | Configuración | Respuesta a comandos de configuración |
| `[VERSION]` | Versión | Respuesta a comando VERSION |
| `[DATA]` | Datos | Streaming de debug (cuando MONITOR está ON) |
| `[ADC]` | Info | Mensaje de inicialización ADC |
| `[OLED]` | Info | Detección de OLED |
| `[SYS]` | Info | Estado del sistema |
| `[USB]` | Info | Eventos USB (mount/unmount/suspend/resume) |

### Caracteres de terminación

- Todos los mensajes terminan con `\r\n` (CR+LF)
- Los comandos aceptan `\r`, `\n`, o ambos como terminador

### Buffer de recepción

- **Tamaño máximo por comando:** 63 caracteres + null terminator
- **Buffer interno:** 64 bytes
- Si se excede el límite, los caracteres adicionales se descartan

---

## 4. Flujo de Conexión Típico

```
1. Conectar al puerto serial (115200 baud)
2. Recibir banner de inicio:
   ========================================
     KY-037 Oscilloscope
     Raspberry Pi Pico RP2040
   ========================================
   [CFG] Default trigger: 1.700V
   [CFG] Commands: TRIG, TRIGMODE, SCALE, TIME, MONITOR, GET, VERSION
   [ADC] KY-037 AO=GP27, DO=GP26 initialized
   [OLED] Found at 0x3C
   [SYS] Ready. Starting main loop...
   ========================================

3. Enviar comandos de configuración:
   > TRIG:2.000
   < [CFG] Trigger set to 2.000V (2484 ADC)

4. Habilitar monitor para ver datos:
   > MONITOR:ON
   < [CFG] Monitor enabled
   < [DATA] ADC= 1800 | V= 1.456V | TRIG=2484 | DO=LOW
   < [DATA] ADC= 2100 | V= 1.699V | TRIG=2484 | DO=HIGH
   ...

5. Consultar configuración:
   > GET
   < [CFG] Trigger=2.000V (2484 ADC) | Mode=LOW | Scale=10.0 | Window=500ms | Monitor=ON | NoiseFloor=1200
```

---

## 5. Ejemplo Python

```python
import serial
import time

# Conectar al Pico
pico = serial.Serial('/dev/ttyACM0', 115200, timeout=2)
time.sleep(2)  # Esperar reset

# Leer banner
for _ in range(10):
    line = pico.readline().decode().strip()
    if line:
        print(f"  {line}")

# Configurar
pico.write(b'TRIG:1.500\r\n')
print(pico.readline().decode().strip())

pico.write(b'TIME:1000\r\n')
print(pico.readline().decode().strip())

# Monitorear datos
pico.write(b'MONITOR:ON\r\n')
print(pico.readline().decode().strip())

try:
    while True:
        line = pico.readline().decode().strip()
        if line and line.startswith('[DATA]'):
            print(line)
except KeyboardInterrupt:
    pico.write(b'MONITOR:OFF\r\n')
    pico.close()
```

---

## 6. Ejemplo Bash (Script de monitoreo)

```bash
#!/bin/bash
PORT="/dev/ttyACM0"

# Configurar puerto
stty -F "$PORT" 115200 raw -echo

# Enviar comandos
echo "TRIG:1.700" > "$PORT"
echo "TIME:500" > "$PORT"
echo "MONITOR:ON" > "$PORT"

# Leer datos por 10 segundos
timeout 10 cat "$PORT"
```

---

## 7. Errores Comunes

| Error | Causa | Solución |
|-------|-------|----------|
| No hay respuesta | Puerto incorrecto | Verificar `/dev/ttyACM0` o usar `ls /dev/ttyACM*` |
| Respuesta vacía | Monitor deshabilitado | Enviar `MONITOR:ON` primero |
| `[CFG] Error` | Parámetro fuera de rango | Verificar rangos en tabla de comandos |
| Datos basura | Baud rate incorrecto | Verificar 115200 baud, 8N1 |
| Buffer lleno | Demasiados comandos seguidos | Esperar respuesta antes de enviar siguiente |
| Dispositivo no encontrado | Firmware no cargado | Repetir flasheo con BOOTSEL |
