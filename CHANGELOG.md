# Changelog — miHMI7

Versión del canal OTA: `MAJOR.MINOR.PATCH` (semver numérico). El firmware embebe
`APP_VERSION`; el CI lo sobreescribe desde el tag `vX.Y.Z` (`FW_VERSION_OVERRIDE`).

## 0.2.0 — hardware verificado en banco + rediseño nativo a 800x480

Todo lo pendiente de v0.1.0 se probó en banco (placa real, `smartpanle`
WT32-S3-WROVER) y se corrigió con datos reales, no con más suposiciones:

- **Se abandonó el lienzo 320x240 escalado por software** (sprite +
  `pushRotateZoom`, ver v0.1.0): en banco se vio con temblor/tearing y texto
  borroso. LVGL ahora dibuja **nativo a 800x480** directo sobre el panel
  (`hal/display.cpp` reescrito, sin sprite intermedio).
- **Bug de color encontrado y resuelto:** el texto/tarjetas salían con
  colores mezclados ("morado", "azul que domina todo") aunque el bus RGB en
  sí estaba bien — confirmado con un test de pantallas solidas (rojo/verde/
  azul/blanco vía `g_lgfx.fillScreen()`, sin pasar por LVGL: salían
  perfectos). Aislado al paso LVGL → `pushImage()`: orden de bytes de cada
  píxel. Corregido con `g_lgfx.setSwapBytes(true)` (no con
  `LV_COLOR_16_SWAP=1`, que arregla lo mismo pero por software y por
  píxel — mucho más lento).
- **Táctil GT911 confirmado funcionando** (I2C, dirección 0x5D) — el
  reporte inicial de "no responde" era la renderización rota (arriba), no el
  driver: con colores/render correctos, los toques se registran bien.
- **"Se siente lento" resuelto (mayormente):** `ModbusTcpSource` (respaldo)
  reintentaba conectar cada 3s a un PLC inalcanzable en la red de banco, y
  `connect()` bloquea — congelaba toda la UI (touch incluido) varios segundos
  cada vez. Throttle subido a 30s (`src/data/modbus_tcp_source.cpp`). Fix
  real pendiente: connect no bloqueante.
- **Fuente de datos por defecto: simulador interno** (`MockSource` como
  primaria, `ModbusTcpSource` como respaldo — antes al revés) para poder
  probar pantalla/táctil en banco sin depender de un PLC real.
- **Rediseño completo de las 9 pantallas** para 800x480 nativo (antes con
  las mismas coordenadas en píxeles que miHMI a 320x240 — ocupaban ~15% de
  la pantalla). Fuentes más grandes (Montserrat 20/28/40/48, antes 12/14/16),
  widgets y botones escalados, sin cambiar la lógica de ninguna pantalla.
- WiFi de banco (`Indetel`) cargada en `include/secrets.h` (fuera de git).

**Pendiente / conocido:**
- El cintillo de mensajes (`lbl_ticker`, detalle de estación) todavía no
  hace scroll de forma confiable pese a angostarlo — falta ver por qué en
  banco antes de seguir ajustando el ancho a ciegas. Se pidió además sumarle
  más datos (RSSI, contador de tramas, alarmas) una vez que el scroll ande.
- Reinicio por USB para flashear: el auto-reset por RTS/DTR no entra solo en
  modo bootloader en esta placa — hace falta BOOT+RESET manual cada vez.

## 0.1.0 — puesta en marcha del hardware (puerto de miHMI a 7")

- Primer scaffold del HMI de 7" sobre la placa **Panlee ZX7D00CE01SV13**
  (WT32-S3-WROVER-N16R8: ESP32-S3, 16MB flash, 8MB PSRAM octal). Pines y
  timings del bus RGB565 tomados del repo oficial del fabricante (placa
  interna "SC05"): https://github.com/smartpanle/PanelLan_esp32_arduino
- **HAL reescrito** (pantalla RGB paralela 800x480 + táctil capacitivo GT911
  por I2C, vía **LovyanGFX** en vez de TFT_eSPI/XPT2046): `hal/display.*`,
  `hal/touch.*`, `hal/panic_screen.cpp`, `hal/lgfx_panel.h` (nuevo),
  `net/ota_hmi.cpp`.
- **Capa de datos/UI portada tal cual** desde `miHMI` (`data/`, `ui/`,
  `net/ota_update.*`, `hal/net_clock.*`, `hal/light_ctrl.*`): sin cambios de
  lógica, solo de hardware subyacente.
- **Decisión de alcance (a propósito, "rápido primero"):** LVGL sigue
  dibujando en un lienzo lógico de 320x240 (mismo layout que `miHMI`, sin
  rediseñar) que se escala 2.5x horizontal / 2.0x vertical al panel físico de
  800x480 con un sprite + `pushRotateZoom` (ver `hal/display.cpp`). Es un
  escalado de bitmap, no una UI nativa a 800x480 — se ve más grande pero algo
  más "en bloques"; un rediseño nativo queda para una fase posterior si hace
  falta más nitidez o aprovechar mejor el espacio.
- **Sin RS485 ni respaldo LoRa** (decisión de alcance): esta placa no trae
  pines dedicados para eso (solo 6 GPIO libres en el conector externo). El
  enlace de campo es exclusivamente Modbus TCP por WiFi — ya era el enlace
  principal en `miHMI`.
- **Sin microSD** (la placa no trae ranura): la configuración vive solo en
  NVS (`data/hmi_config.cpp` simplificado, sin el respaldo/espejo en tarjeta
  que sí tiene `miHMI`).
- **Sin LDR de fábrica**: `light.enabled = 0` por defecto (backlight fijo en
  `blManual`). `PIN_LDR` queda apuntando a un GPIO libre del conector externo
  por si se cablea uno a futuro — el pipeline de `hal/light_ctrl.cpp` es el
  mismo de `miHMI`, sin cambios.
- Partición dual-OTA propia para 16MB de flash (`partitions_16mb_ota.csv`):
  app0/app1 de 6MB cada una, LittleFS de ~3.9MB (reemplaza el uso que
  `miHMI` le daba a la microSD).
- **Pendiente de verificar en hardware real** (no se pudo probar sin el
  equipo en mano): orientación/polaridad exacta del bus RGB, si hace falta
  `LV_COLOR_16_SWAP=1` o `canvas.setSwapBytes(true)` para el orden de color
  correcto, y el mapeo de ejes del táctil (por si la placa monta el panel
  espejado en algún eje).
