# Changelog — miHMI7

Versión del canal OTA: `MAJOR.MINOR.PATCH` (semver numérico). El firmware embebe
`APP_VERSION`; el CI lo sobreescribe desde el tag `vX.Y.Z` (`FW_VERSION_OVERRIDE`).

## 0.2.4 — OTA sin pantalla negra: recuadro chico sobre el fondo congelado

- **Se quito el `fillScreen()` completo del OTA** (pedido de banco: la
  pantalla se ponia negra entera al buscar/aplicar una actualizacion).
  `net/ota_hmi.cpp` ahora solo dibuja un recuadro fijo de 420x130 centrado
  -- el resto de la pantalla queda "congelado" tal cual la dejo LVGL (el
  loop principal no vuelve a llamar `lv_timer_handler()` mientras el OTA
  esta corriendo, asi que no hay conflicto con lo que ya esta dibujado).
  Al terminar, `lv_obj_invalidate(lv_scr_act())` fuerza a LVGL a repintar
  todo y el recuadro desaparece.
- **El porcentaje ahora va DENTRO de la barra**, no debajo -- mismo cuidado
  de v0.2.3 para que el digito nuevo tape siempre al viejo sin fantasma.

## 0.2.3 — barra de progreso del OTA: fin del "fantasma" de texto

- **La barra de progreso (%) del OTA seguia viendose mal en banco tras el
  fix de fuente de v0.2.1** (video: "83%" con un digito fantasma superpuesto
  al lado). No era un problema de fuente: `net/ota_hmi.cpp::draw()` hacia un
  `fillScreen()` de la pantalla COMPLETA (800x480) en cada punto de
  porcentaje durante la descarga -- hasta 100 veces, varias por segundo. El
  panel RGB de esta placa no tiene doble buffer (el DMA lo escanea en vivo
  desde el mismo buffer que la CPU esta escribiendo), asi que un redibujo
  tan grande y tan frecuente se veia "partido" a mitad de escritura
  (tearing) -- eso es lo que la camara capturo como texto superpuesto.
  Arreglado: el fondo (titulo, mensaje, marco de la barra) se pinta UNA
  sola vez al entrar a la fase de descarga; cada tick de porcentaje ahora
  solo toca el interior de la barra + una caja de texto de ancho fijo
  (evita tambien que un digito nuevo mas corto deje asomando el viejo).
  Mucho menos que escribir para el panel en cada actualizacion.

## 0.2.2 — IP/RSSI en diagnostico, OTA cada 5 min, menos ruido serie

- **IP y RSSI del HMI no se mostraban en Ajustes/Diagnostico**: se pedian a
  `DataHub::primary()->localIp()/linkRssi()`, pero solo `ModbusTcpSource`
  los implementa de verdad (el resto de `DataSource` devuelve "-"/0 por
  defecto) — y la fuente primaria por defecto es `MockSource` (simulador,
  ver v0.2.0). `screen_settings.cpp` ahora lee `WiFi.status()/localIP()/
  RSSI()` directo: es un dato del equipo, no de cual fuente esta activa.
- **Boton "Recalibr." quitado** de Ajustes: el GT911 es tactil capacitivo
  calibrado de fabrica, no tiene rutina de recalibracion — el boton
  (heredado de miHMI/XPT2046 resistivo) no hacia nada (reporte de banco).
- **Chequeo de OTA cada 5 min** (antes 6 h) — `net/ota_hmi.cpp`.
- **Menos ruido en el puerto serie**: se quito el log de `disp_flush()`
  (corria al menos 1 vez/seg sin parar) y el de PRESS/release del tactil en
  cada toque, mas el escaneo I2C completo en cada arranque — todos eran
  diagnosticos de puesta en marcha ya resueltos (ver v0.1.0/v0.2.0). Los
  comandos por consola (`light`, `theme`, `ldr`, `time`, `depth`, `inv`)
  siguen igual, y se agrego `plc`: vuelca la tabla actual leida de la
  fuente activa (por estacion: nivel/caudal/acumulados/digitales/alarmas/
  rssi) mas el estado global del enlace, a pedido en vez de automatico.

## 0.2.1 — repo público (OTA), fix de fuente y cintillo con scroll forzado

- **Repo pasado a público** (`asdrubalfuentes/miHMI7`): el OTA descarga
  `version.txt`/`firmware.bin` de un GitHub Release por HTTPS sin
  autenticación — contra un repo privado eso da 404, y era la causa real de
  "no se pudo leer version.txt" al buscar actualización (no era la red).
- **Bug de fuente encontrado y corregido** (`net/ota_hmi.cpp`,
  `hal/panic_screen.cpp`): ambos usaban el índice de fuente heredado "7" de
  LovyanGFX para texto normal — pero ese índice es el set **7 segmentos**
  (solo dígitos, sin letras). Con texto real ("Actualizacion de firmware",
  títulos de `panic_screen`) el ancho calculado salía mal y el texto/la
  barra de progreso se salían de la pantalla. Cambiado a Font 4 (alfabeto
  completo) + `setTextSize()` para mantener el tamaño grande.
- **Cintillo de mensajes: scroll forzado**, no el automático de LVGL
  (`LV_LABEL_LONG_SCROLL_CIRCULAR`, que resultó poco confiable en banco —
  a veces no se movía, o se movía pero pegado a la izquierda). Ahora es una
  animación propia (`lv_anim`) que desliza el texto de derecha a izquierda
  sin parar, siempre, sin depender de que LVGL decida que "desborda".
- **Cintillo con más datos de la estación**: RSSI, contador de tramas
  OK/ERR y estado de alarmas (antes solo nombre/hora/estado de enlace).

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
