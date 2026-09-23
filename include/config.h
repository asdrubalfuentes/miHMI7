/**
 * config.h  -  Pines, tiempos, flags y mapa Modbus (placeholder) del HMI de 7".
 *
 * Placa: Panlee ZX7D00CE01SV13 (modelo interno del fabricante "SC05"),
 *        WT32-S3-WROVER-N16R8 (ESP32-S3, 16MB flash, 8MB PSRAM octal).
 * Pines segun el repo oficial del fabricante (Shenzhen QM Smart Panlee):
 *   https://github.com/smartpanle/PanelLan_esp32_arduino/blob/master/src/board/sc05/sc05_pin.h
 *
 * Diferencias de hardware frente a miHMI (Cheap Yellow Display 320x240):
 *   - Pantalla RGB565 paralela 800x480 (no SPI) -> LovyanGFX en vez de TFT_eSPI.
 *   - Tactil capacitivo GT911 por I2C (no XPT2046 resistivo) -> sin calibracion.
 *   - Sin ranura microSD, sin RS485, sin UART dedicado a LoRa -> esta version
 *     solo usa Modbus TCP por WiFi (igual que ya era el enlace principal en
 *     miHMI); la config persiste solo en NVS (ver data/hmi_config.cpp).
 *
 * v0.2.0: se abandono el primer intento (lienzo logico 320x240 escalado por
 * software a 800x480) -- en banco se vio con temblor/tearing y las letras
 * salian con fleco de color (separacion R/G/B en los bordes). LVGL ahora
 * dibuja NATIVO a 800x480 directo sobre el panel (ver hal/display.cpp); el
 * fleco de color que se ve en banco es mas compatible con timing/skew del
 * bus RGB que con el escalado de software (que ya no existe), asi que si
 * persiste el proximo paso es bajar freq_write en hal/lgfx_panel.h.
 */
#pragma once

/* ============================ Branding ============================ */
#define APP_NAME        "HMI Captacion de Pozos"
#define APP_VERSION     "0.2.1"
/* El CI (.github/workflows/release.yml) define FW_VERSION_OVERRIDE = X.Y.Z del
 * tag; esa es la version que compara el cliente OTA (net/ota_hmi). */
#ifdef FW_VERSION_OVERRIDE
#  undef  APP_VERSION
#  define APP_VERSION FW_VERSION_OVERRIDE
#endif
#define CLIENT_NAME     "CMSG PSL"          /* TODO: logo real del cliente */
#define PRODUCT_NAME    "AYSAFI  -  Ingenieria y Tecnologia"

/* ===================== OTA (GitHub Releases pull) ============== */
/* Un tag vX.Y.Z sobre main -> Release "latest" con firmware.bin + version.txt +
 * firmware.sha256. Particion dual-OTA propia (ver partitions_16mb_ota.csv). */
#define OTA_GH_OWNER    "asdrubalfuentes"
#define OTA_GH_REPO     "miHMI7"

/* ============================ Pantalla =========================== */
/* 800x480 RGB565 paralelo, LVGL dibuja NATIVO a esta resolucion (sin sprite
 * ni escalado -- ver hal/lgfx_panel.h para el bus/timings del panel). */
#define SCREEN_W        800
#define SCREEN_H        480
/* Buffer parcial de LVGL en PSRAM (ver hal/display.cpp): 800*120*2 = 187.5KB.
 * Grande a proposito -- pocas llamadas a disp_flush() por pantalla en vez de
 * muchas chicas, cada pushImage() al panel RGB tiene su propio costo fijo. */
#define DRAW_BUF_LINES  120

#define PIN_LCD_BL          45      /* backlight, PWM (LovyanGFX Light_PWM) */
#define PIN_LCD_PCLK        9
#define PIN_LCD_VSYNC       38
#define PIN_LCD_HSYNC       5
#define PIN_LCD_DE          39
/* D0..D15 del bus RGB565: B0-4, G0-5, R0-4 (orden y pines del fabricante) */
#define PIN_LCD_D0          17   /* B0 */
#define PIN_LCD_D1          16   /* B1 */
#define PIN_LCD_D2          15   /* B2 */
#define PIN_LCD_D3          7    /* B3 */
#define PIN_LCD_D4          6    /* B4 */
#define PIN_LCD_D5          21   /* G0 */
#define PIN_LCD_D6          0    /* G1 */
#define PIN_LCD_D7          46   /* G2 */
#define PIN_LCD_D8          3    /* G3 */
#define PIN_LCD_D9          8    /* G4 */
#define PIN_LCD_D10         18   /* G5 */
#define PIN_LCD_D11         10   /* R0 */
#define PIN_LCD_D12         11   /* R1 */
#define PIN_LCD_D13         12   /* R2 */
#define PIN_LCD_D14         13   /* R3 */
#define PIN_LCD_D15         14   /* R4 */

/* ===================== Tactil GT911 (I2C, capacitivo) ============ */
/* Bus I2C compartido de la placa (tactil, sin otros perifericos I2C hoy). */
#define PIN_I2C_SDA     48
#define PIN_I2C_SCL     47
#define GT911_I2C_ADDR  0x5D
#define GT911_I2C_FREQ  400000

/* ===================== Perifericos de la placa ================== */
/* Esta placa no trae LED RGB de estado ni fotoresistencia (LDR) de fabrica.
 * PIN_LDR queda apuntando a un GPIO libre del conector externo (ADC1, sin
 * usar hoy) por si se cablea un LDR a futuro -- con light.enabled=0 (default
 * de esta version, ver hmi_config.cpp) el modulo hal/light_ctrl no lo toca
 * para nada, el backlight queda fijo en blManual. */
#define PIN_LDR         4        /* EXTERNAL_PIN_4 del conector, libre */

/* Reinicios anormales (panic/watchdog/brownout) seguidos que se toleran antes
 * de arrancar en MODO BASICO: sin ajustes guardados, solo defaults. */
#define BOOT_MAX_FAILS  3
#define BOOT_STABLE_MS  15000   /* uptime sin caer -> se da el arranque por bueno */

/* ========================= Temporizaciones ===================== */
#define SPLASH_MS       2500
#define LVGL_TASK_MS    5
#define APP_TICK_MS     250

/* ========================= Feature flags ====================== */
#define FEATURE_MQTT        0
#define FEATURE_LORA_BACKUP 0    /* esta placa no tiene UART/RS485 dedicado; solo Modbus TCP por WiFi */
#define FEATURE_SD_LOG      0    /* sin ranura microSD en esta placa */

/* ===================== Pozos / estaciones ===================== */
#define NUM_WELLS       2       /* 2 estaciones de bombeo (contrato Mapa B) */
#define WELL_NAME_LEN   16

/* ===================== Historico de caudal ==================== */
#define HIST_DAYS       14      /* barras del grafico "ultimos dias" */
#define HIST_MONTHS     6       /* barras del grafico "ultimos meses" */

/* ====== Enlace de campo por Modbus TCP (MAPA B del contrato) ======= */
/* Contrato: ../ORCHESTRATION/REGISTER_MAP.md   ·   offsets en src/data/map_b.h */
#define CONTRACT_VERSION     3
#define MAP_B_WORD_HI_FIRST  1        /* 32b: palabra alta en la dir. menor (contrato Sec. 2) */

/* Credenciales WiFi: fuera del control de versiones.  Copia
 * include/secrets.example.h a include/secrets.h y pon las de tu planta. */
#if defined(__has_include)
#  if __has_include("secrets.h")
#    include "secrets.h"
#  endif
#endif
#ifndef WIFI_SSID
#  define WIFI_SSID          ""       /* vacio = no conecta (queda la fuente de respaldo) */
#endif
#ifndef WIFI_PASS
#  define WIFI_PASS          ""
#endif

#define PLC_HOST             "192.168.1.56"  /* IP del LOGO! 9 real o del PLC-SIM (por defecto; NVS manda si hay) */
#define PLC_PORT             503      /* LOGO! 9 de planta hoy en :503 (por defecto) */
#define PLC_UNIT             1        /* Unit ID (el servidor responde con cualquiera) */
#define MB_POLL_MS           1000     /* periodo de sondeo del Mapa B (por defecto) */

#define HMI_ADMIN_PIN_DEFAULT "1234"  /* PIN de administrador de fabrica; cambialo en Configuracion */

#define MB_LEVEL_SCALE       100.0f   /* Mapa B: nivel y caudal viajan x100 */
#define MB_FLOW_SCALE        100.0f
#define MB_ACCUM_SCALE       1000.0f  /* acumulados en m3 x1000 (nodo remoto, sin conversion en el LOGO!) */
