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
 *   - La UI se porta TAL CUAL (mismo layout que miHMI, 320x240 logico) y se
 *     escala 2.5x/2.0x a los 800x480 fisicos (ver hal/display.cpp) -- opcion
 *     "rapido primero" elegida a proposito; un rediseno nativo a 800x480
 *     queda para una fase posterior si hace falta.
 */
#pragma once

/* ============================ Branding ============================ */
#define APP_NAME        "HMI Captacion de Pozos"
#define APP_VERSION     "0.1.0"
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
/* Fisica: 800x480 RGB565 paralelo (ver hal/lgfx_panel.h para el bus/timings).
 * Logica para LVGL: se mantiene 320x240 (igual que miHMI) y hal/display.cpp
 * la escala a la pantalla real con un sprite + pushRotateZoom (zoom no
 * uniforme: 2.5x horizontal, 2.0x vertical -- 800/320 y 480/240 exactos).
 * Ver nota de arriba: layout SIN redisenar todavia, a proposito. */
#define SCREEN_W        320       /* resolucion LOGICA (la que usa toda la UI/LVGL) */
#define SCREEN_H        240
#define PHYS_SCREEN_W   800       /* resolucion FISICA del panel */
#define PHYS_SCREEN_H   480
#define UI_ZOOM_X       (float)PHYS_SCREEN_W / (float)SCREEN_W   /* 2.5 */
#define UI_ZOOM_Y       (float)PHYS_SCREEN_H / (float)SCREEN_H   /* 2.0 */
#define DRAW_BUF_LINES  32       /* alto del buffer parcial de LVGL (igual que miHMI) */

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
