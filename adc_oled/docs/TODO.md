# TODO.md - ADC + OLED KY-037

Lista de tareas pendientes y mejoras para el proyecto osciloscopio digital.

---

## 🔴 Crítico (Bugs que afectan funcionamiento)

- [ ] **FIX: `adc_sum` overflow en `uint16_t`**
  - `adc_sum` es `uint16_t` (max 65535) pero se acumulan ~50 muestras a ~2000 ADC = ~100,000
  - Solución: cambiar a `uint32_t`
  - Archivo: `src/main.c` línea ~60

- [ ] **FIX: Comentario incorrecto "512ms" en header**
  - Línea 7 dice "512ms de ventana" pero `scope_window_ms = 500`
  - Solución: corregir comentario a "500ms"

- [ ] **FIX: `SSD1306_send_buf` sin NULL check en malloc**
  - `malloc(buflen + 1)` puede retornar NULL en microcontrolador
  - Solución: agregar check `if (!temp_buf) return;`

- [ ] **FIX: Macros `SSD1306_SET_COM_OUT_DIR` y `_FLIP` idénticas**
  - Ambas definen `0xC0`, la flip debería ser `0xC8`
  - Solución: definir correctamente o eliminar la duplicada

---

## 🟡 Mejoras Funcionales

- [ ] **Eliminar `malloc/free` en `SSD1306_send_buf`**
  - Se llama cada 200ms, fragmenta memoria en MCU
  - Solución: usar buffer estático o buffer en stack con tamaño conocido

- [ ] **Variable `trigger_scale` declarada pero nunca usada en render**
  - Se puede configurar por serial `SCALE:` pero no afecta la visualización
  - Solución: implementar zoom en el waveform o eliminar la variable

- [ ] **Validar input de `atof()` en comandos seriales**
  - `TRIG:abc` convierte a 0.0 silenciosamente
  - Solución: validar que el string sea numérico antes de convertir

- [ ] **`cmd_buf` estática sin limpieza en reconexión USB**
  - Al reconectar USB, buffers parciales pueden persistir
  - Solución: limpiar `cmd_len = 0` en `tud_mount_cb()`

- [ ] **`noise_floor` se incrementa de 1 en 1 (extremadamente lento)**
  - A 250Hz, tarda ~17 segundos en subir 100 unidades
  - Solución: incremento proporcional o configurable

- [ ] **`cdc_send_string` hace flush inmediato**
  - Puede causar overhead si el buffer TX está lleno
  - Solución: flush solo cuando sea necesario o usar buffer

---

## 🟢 Nuevas Funcionalidades

- [ ] **Canal dual ADC** — Leer GP28 (ADC2) simultáneamente
  - Mostrar dos traces en el OLED o alternar entre ellos
  - Requiere: segundo buffer, segundo muestreo, UI para seleccionar

- [ ] **Botones hardware** para cambiar configuración
  - Usar GPIO libres (GP7–GP15, GP18–GP22)
  - Funciones: cambiar trigger, escala, ventana de tiempo
  - Requiere: debounce software

- [ ] **FFT básica** para análisis de frecuencia
  - Usar las 128 muestras del buffer
  - Mostrar espectro en el OLED (bars display)
  - Requiere: tabla de seno/coseno, algoritmo FFT radix-2

- [ ] **Almacenamiento de datos en flash**
  - Guardar últimas N mediciones en flash interno
  - Comando serial `SAVE` / `LOAD`
  - Requiere: gestión de flash, wear leveling

- [ ] **Exportación JSON por USB**
  - Usar la librería nlohmann ya incluida (no utilizada)
  - Comando `EXPORT` envía datos en formato JSON
  - Compatible con herramientas de análisis en PC

- [ ] **Display de barras (VU Meter)**
  - Modo alternativo al waveform
  - Mostrar nivel de sonido como barra vertical
  - Cambiar con botón o comando serial

- [ ] **Modo "single shot"**
  - Capturar una sola triggered wave y congelar display
  - Comando `SINGLE` para activar

- [ ] **Configuración persistente en flash**
  - Guardar trigger, escala, ventana en flash
  - Cargar al inicio con `LOAD_CONFIG`
  - Requiere: Flash functions del SDK

- [ ] **Protocolo de comunicación bidireccional**
  - Respuestas JSON completas para cada comando
  - Streaming de datos en formato estructurado
  - Compatible con Python/Node.js en PC

---

## 🔧 Mantenimiento

- [ ] **Separar driver SSD1306 en archivo propio**
  - Mover funciones SSD1306 de `main.c` a `ssd1306.c` / `ssd1306.h`
  - Mejorar modularidad del código

- [ ] **Agregar unit tests**
  - Tests para `process_serial_command()`
  - Tests para `detect_trigger()`
  - Tests para funciones de dibujo (SetPixel, DrawLine)
  - Usar CUnit o framework ligero

- [ ] **Documentar función `SSD1306_scroll()`**
  - Función existe pero nunca se llama
  - Evaluar si es necesaria o eliminar

- [ ] **Revisar `usb_descriptors.h`**
  - Header vacío (solo include guard)
  - Puede eliminarse o agregar prototipos

- [ ] **Revisar carpeta `nlohmann/`**
  - Contiene JSON library no utilizada
  - Decidir: usar para export o eliminar para reducir tamaño

- [ ] **Actualizar `.gitignore`**
  - Agregar `*.bak` para excluir `json.hpp.bak`
  - Agregar `compile_commands.json`

- [ ] **Script `install_deps.sh` requiere sudo**
  - Documentar claramente que necesita root
  - O usar `--user` flag donde sea posible

---

## 📊 Estadísticas

| Categoría | Total | Completado |
|-----------|-------|------------|
| 🔴 Crítico | 4 | 0 |
| 🟡 Mejoras | 6 | 0 |
| 🟢 Nuevas features | 9 | 0 |
| 🔧 Mantenimiento | 7 | 0 |
| **Total** | **26** | **0** |

---

## Prioridad de Implementación

```
1. adc_sum overflow (crítico, rompe display)
2. SSD1306_send_buf malloc (estabilidad)
3. Macros duplicadas (claridad)
4. Comentario 512ms (cosmético)
5. Validar input serial (UX)
6. trigger_scale funcional (completar feature)
7. Separar SSD1306 (modularidad)
8. Unit tests (calidad)
```
