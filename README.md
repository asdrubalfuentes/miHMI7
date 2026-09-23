# miHMI7

HMI de captación de pozos profundos — versión de **7"** de [`miHMI`](../miHMI),
sobre la placa **Panlee ZX7D00CE01SV13** (chip **WT32-S3-WROVER-N16R8**:
ESP32-S3, 16MB flash, 8MB PSRAM octal).

Mismo contrato Modbus TCP (`ORCHESTRATION/REGISTER_MAP.md`, Mapa B) y misma
capa de aplicación que `miHMI`; lo que cambia es el hardware de pantalla/táctil
y la ausencia de microSD/RS485/LoRa en esta placa. Ver el detalle completo en
[`CHANGELOG.md`](CHANGELOG.md).

## Hardware

| | miHMI (CYD) | miHMI7 (esta placa) |
|---|---|---|
| MCU | ESP32 (sin PSRAM) | ESP32-S3, 16MB flash, 8MB PSRAM |
| Pantalla | ILI9341 320x240, SPI | RGB565 paralelo 800x480 |
| Táctil | XPT2046 resistivo (SPI, con calibración) | GT911 capacitivo (I2C, sin calibración) |
| Librería gráfica | TFT_eSPI | LovyanGFX |
| microSD | Sí (config + histórico) | No — todo en NVS |
| RS485 / LoRa backup | Sí (UART dedicado) | No — solo Modbus TCP por WiFi |

Pines del panel y timings del bus RGB: repo oficial del fabricante (placa
interna "SC05" en su librería), único origen consultado para el pinout:
https://github.com/smartpanle/PanelLan_esp32_arduino/blob/master/src/board/sc05/sc05_pin.h

## Layout de la UI: escalado, no rediseñado (todavía)

LVGL sigue dibujando en un lienzo lógico de **320x240** — el mismo tamaño que
`miHMI` — y `hal/display.cpp` lo escala 2.5x horizontal / 2.0x vertical al
panel físico de 800x480 (sprite en PSRAM + `pushRotateZoom`). Es la opción
"rápido primero": mismo código de pantallas (`src/ui/*`) sin tocar, hardware
funcionando antes. El resultado se ve más grande pero un poco más "en
bloques" que un dibujo nativo a 800x480 — un rediseño que aproveche el
espacio real queda para una fase posterior.

## Estructura

Igual que `miHMI` salvo:
- `src/hal/lgfx_panel.h` — nuevo: definición LovyanGFX del panel RGB + GT911.
- `src/hal/display.*`, `src/hal/touch.*`, `src/hal/panic_screen.cpp`,
  `src/net/ota_hmi.cpp` — reescritos para LovyanGFX (antes TFT_eSPI/XPT2046).
- `src/data/hmi_config.cpp` — simplificado a NVS-only (sin microSD).
- `partitions_16mb_ota.csv` — partición dual-OTA propia para 16MB de flash.

## Build

```bash
pio run                        # compilar
pio run -t upload --upload-port COMx   # flashear por USB
```

## OTA

Igual mecanismo que el resto del stack Aysafi: un tag `vX.Y.Z` sobre `main`
dispara `.github/workflows/release.yml`, que publica un GitHub Release
(`firmware.bin` + `version.txt` + `firmware.sha256`) que el equipo consulta
en el próximo arranque / chequeo periódico (`net/ota_hmi.cpp`).

## Pendiente de verificar en hardware real

No se pudo probar en el equipo físico durante el armado de este proyecto.
Antes de darlo por bueno en banco, revisar:
- Orden de color del bus RGB (si sale con rojo/azul invertido, ver la nota en
  `include/lv_conf.h` / `hal/display.cpp`).
- Orientación y mapeo de ejes del táctil GT911.
- Que el brillo (`hal/display.cpp::display_backlight_pct`) responda bien en
  todo el rango 0–100%.
