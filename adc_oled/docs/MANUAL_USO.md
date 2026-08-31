# Manual de Uso - ADC + OLED SSD1306 con KY-037

Visualizador de nivel de sonido en tiempo real con Raspberry Pi Pico, sensor KY-037 y display OLED.

---

## 1. ¿Qué es esto?

Un dispositivo que lee el nivel de sonido desde un micrófono (módulo KY-037) y lo muestra en una pantalla OLED en tiempo real. Además, envía datos de debug por USB CDC (Virtual Serial Port) para monitorear el funcionamiento.

---

## 2. Componentes Necesarios

| Componente | Cantidad | Dónde conseguirlo |
|------------|----------|-------------------|
| Raspberry Pi Pico | 1 | Tiendas de electrónica (~$4 USD) |
| Módulo KY-037 | 1 | Sensor de detección de sonido (~$2 USD) |
| Display OLED SSD1306 | 1 | AliExpress, Amazon, tiendas (~$3 USD) |
| Protoboard | 1 | Tienda de electrónica (~$2 USD) |
| Cable jumper M-M | 6 | Tienda de electrónica (~$1 USD) |
| Cable USB micro-B | 1 | El que trae el Pico |

**Total aproximado:** ~$12 USD

---

## 3. Armado del Hardware

### Paso 1: Conectar el módulo KY-037

Conecta el módulo KY-037 de la siguiente manera:

| Pin KY-037 | Pin Pico | GPIO | Función |
|-------------|----------|------|---------|
| VCC | Pin 37 | 3V3 | Alimentación 3.3V |
| GND | Pin 35 | GND | Tierra |
| AO | Pin 33 | GP27 | Salida analógica (ADC1) |
| DO | Pin 32 | GP26 | Salida digital |

**Ajuste de sensibilidad:** El KY-037 tiene un potenciómetro integrado (marcado como "Sensitivity" o "Threshold"). Gíralo para ajustar la sensibilidad:
- Sentido horario: más sensible (detecta sonidos más suaves)
- Sentido antihorario: menos sensible (solo detecta sonidos fuertes)

### Paso 2: Conectar el OLED

Conecta el display OLED al I2C0:

| Pin OLED | Pin Pico | GPIO | Función |
|----------|----------|------|---------|
| GND | Pin 35 | GND | Tierra |
| VCC | Pin 37 | 3V3 | Alimentación 3.3V |
| SDA | Pin 25 | GP16 | Datos I2C0 |
| SCL | Pin 26 | GP17 | Reloj I2C0 |

### Paso 3: Verificar conexiones

```
KY-037
═══════════════════════════════════════════════════════
VCC ──────────── 3V3 (Pin 37)
GND ──────────── GND (Pin 35)
AO  ──────────── GP27 (Pin 33) ◄── Analog
DO  ──────────── GP26 (Pin 32) ◄── Digital

OLED SSD1306
═══════════════════════════════════════════════════════
GND ──────────── GND (Pin 35)
VCC ──────────── 3V3 (Pin 37)
SDA ──────────── GP16 (Pin 25)
SCL ──────────── GP17 (Pin 26)
```

**Importante:** Verifica que no haya cortocircuitos y que los cables estén bien conectados antes de alimentar el Pico.

---

## 4. Instalar el Firmware

### Requisitos previos

- Computadora con Windows, macOS o Linux
- Cable USB
- Pico SDK instalado

### Paso 1: Compilar

```bash
cd pico_src/adc_oled
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

### Paso 2: Poner el Pico en modo programación

1. **Desconectar** el Pico del USB
2. **Mantener presionado** el botón blanco BOOTSEL
3. **Conectar** el Pico al USB mientras mantienes el botón
4. **Soltar** el botón cuando la luz LED se encienda

Aparece una unidad USB llamada `RPI-RP2` o `RP2040`.

### Paso 3: Copiar el firmware

**Windows:**
```
Copia el archivo adc_oled.uf2 a la unidad RPI-RP2
```

**macOS:**
```bash
cp pico_src/adc_oled/build/adc_oled.uf2 /Volumes/RPI-RP2/
```

**Linux:**
```bash
cp pico_src/adc_oled/build/adc_oled.uf2 /media/$USER/RPI-RP2/
```

### Paso 4: Reiniciar

El Pico se reinicia automáticamente. El display OLED debe encenderse y mostrar los valores del ADC.

---

## 5. Cómo Usar

### Funcionamiento normal

1. El KY-037 detecta sonidos ambiente a través de su micrófono electret
2. El OLED muestra en tiempo real:
   - **ADC**: valor analógico de 0 a 4095 (intensidad de sonido)
   - **V**: voltaje correspondiente (0.00V a 3.30V)
   - **DO**: estado digital (HIGH/LOW) según el umbral ajustado
   - **Barra**: representación gráfica del nivel de sonido

### Monitor Serial USB (CDC)

El Pico se conecta por USB y aparece como un puerto serial virtual:

| Sistema | Puerto |
|---------|--------|
| Linux | `/dev/ttyACM0` |
| Windows | `COMx` (ver Administrador de dispositivos) |
| macOS | `/dev/cu.usbmodem...` |

**Datos enviados (cada 500ms):**
```
========================================
  ADC + OLED + KY-037
  Raspberry Pi Pico RP2040
========================================
[ADC] KY-037 AO=GP27, DO=GP26 initialized
[OLED] Found at 0x3C
[OLED] Initialized OK
[SYS] Ready. Starting main loop...
========================================
[DATA] ADC= 1234 | V= 1.23V | DO=LOW  | OLED=OK
[DATA] ADC= 2567 | V= 2.56V | DO=HIGH | OLED=OK
```

**Para ver los datos:**

Linux:
```bash
sudo apt install minicom
minicom -D /dev/ttyACM0 -b 115200
```

Windows:
- Abrir Administrador de dispositivos
- Buscar el puerto COM del Pico
- Usar PuTTY o Arduino Serial Monitor

macOS:
```bash
screen /dev/cu.usbmodem... 115200
```

### Detección de sonido

- **DO = LOW**: No se detectó sonido por encima del umbral
- **DO = HIGH**: Se detectó sonido por encima del umbral

El umbral se ajusta con el potenciómetro integrado en el KY-037.

### Ajustar la sensibilidad

1. Gira el potenciómetro del KY-037 lentamente
2. Prueba hacer un sonido (aplaudir, hablar)
3. Ajusta hasta que el LED del KY-037 se encienda con el nivel de sonido deseado
4. El valor DO en el OLED cambiará de LOW a HIGH cuando se detecte el sonido

---

## 6. Modificar el Código

### Cambiar el pin ADC (KY-037)

En `src/main.c`, modifica:

```c
#define ADC_PIN            27      // GPIO 27 (ADC1) - KY-037 AO
#define ADC_CHANNEL        1       // ADC channel 1
```

Para usar GPIO 26 (ADC0) en lugar de GPIO 27:
```c
#define ADC_PIN            26
#define ADC_CHANNEL        0
```

### Cambiar pines I2C (OLED)

En `src/main.c`, modifica:

```c
#define I2C_PORT           i2c0
#define I2C_SDA_PIN        16      // GPIO 16 (I2C0 SDA)
#define I2C_SCL_PIN        17      // GPIO 17 (I2C0 SCL)
```

Para usar I2C1 con GPIO 2 y 3:
```c
#define I2C_PORT           i2c1
#define I2C_SDA_PIN        2
#define I2C_SCL_PIN        3
```

### Cambiar la dirección I2C del OLED

```c
#define OLED_I2C_ADDR      0x3C    // Cambiar a 0x3D si corresponde
```

### Cambiar velocidad de actualización

```c
#define DISPLAY_UPDATE_MS  50     // 50ms = 20 FPS
```

---

## 7. Solución de Problemas

### El OLED no enciende

1. **Verificar alimentación:** ¿3.3V y GND están conectados?
2. **Verificar I2C:** ¿SDA (GP16) y SCL (GP17) están en los pines correctos?
3. **Verificar dirección I2C:** Algunos OLEDs usan 0x3D en lugar de 0x3C
4. **Verificar driver:** Algunos OLEDs necesitan un reset externo

### El OLED enciende pero no muestra nada

1. **Verificar contraste:** En `main.c`, modifica `ssd1306_send_command(SSD1306_CMD_SET_CONTRAST)` con un valor diferente (0x00 a 0xFF)
2. **Verificar inicialización:** Asegúrate que el OLED recibe los comandos de inicialización
3. **Verificar I2C con osciloscopio o logic analyzer**

### El KY-037 no detecta sonido

1. **Verificar alimentación:** ¿VCC (3.3V) y GND están conectados?
2. **Verificar pines:** ¿AO está en GP27 y DO en GP26?
3. **Ajustar sensibilidad:** Gira el potenciómetro del KY-037 hasta detectar sonido
4. **Verificar micrófono:** El KY-037 tiene un micrófono electret, asegúrate que no esté dañado
5. **Verificar LED:** El KY-037 tiene un LED que se enciende cuando detecta sonido

### El valor analógico no cambia

1. **Verificar conexiones del KY-037:** VCC, GND, AO y DO
2. **Verificar pin ADC:** Debe ser GPIO 27 (canal 1)
3. **Ajustar sensibilidad:** Gira el potenciómetro del KY-037
4. **Verificar que haya sonido:** El micrófono necesita sonido ambiente para detectar

### El display parpadea o tiene ruido

1. **Usar cables más cortos** para I2C
2. **Agregar capacitores de desacople** (100nF) cerca del OLED
3. **Reducir la velocidad I2C:** Cambiar `I2C_BAUDRATE` a 100000 (100 kHz)

---

## 8. Especificaciones Técnicas

| Parámetro | Valor |
|-----------|-------|
| MCU | RP2040 (dual core ARM Cortex-M0+) |
| Frecuencia | 125 MHz |
| ADC | 12 bits (0-4095) |
| Canales ADC | 4 (GPIO 26-29) |
| I2C | 2 controladores (I2C0, I2C1) |
| OLED | SSD1306, 128x64 pixels |
| Velocidad I2C | 400 kHz (fast mode) |
| Sensor | KY-037 (micrófono electret) |
| USB CDC | Virtual Serial Port (115200 baud) |
| Voltaje de operación | 5V (USB) o 1.8-5.5V (VSYS) |
| Consumo típico | ~30-50 mA |

---

## 9. Limitaciones

- **1 canal ADC** en este ejemplo (se puede expandir a 4)
- **Sin almacenamiento** de datos
- **Sin conectividad** USB/inalámbrica
- **Display monocromático** (blanco y negro)
- **Umbral digital fijo** (ajustable solo con potenciómetro hardware)

---

## 10. Próximas Mejoras Posibles

- [ ] Agregar más canales ADC para múltiples sensores
- [ ] Agregar botón para cambiar modo de visualización
- [ ] Mostrar gráficos de serie temporal en el OLED
- [ ] Guardar valores máximos/mínimos en memoria
- [ ] Agregar alertas visuales (umbrales)
- [ ] Medir frecuencia de sonido (FFT básica)
- [ ] Conectar por USB como dispositivo HID

---

## 11. Licencia

MIT License - Libre para uso personal y comercial.
