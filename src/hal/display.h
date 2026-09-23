/**
 * display.h  -  Pantalla RGB565 800x480 (LovyanGFX) + enlace con LVGL.
 *
 * LVGL sigue dibujando en un lienzo LOGICO de 320x240 (igual que miHMI, sin
 * cambios en ui/*) que se escala 2.5x/2.0x al panel fisico -- ver display.cpp.
 */
#pragma once
#include <Arduino.h>
#include "lgfx_panel.h"

/* Inicializa el panel (bus RGB, backlight ON) y el lienzo logico 320x240. */
void display_hw_init();

/* Registra el driver de display y los buffers en LVGL. Llamar tras lv_init(). */
void display_lvgl_init();

/* Acceso al objeto LGFX real (pantalla FISICA 800x480) -- lo usan panic_screen
 * y ota_hmi para dibujar pantallas de emergencia/progreso fuera de LVGL. */
LGFX &display_lgfx();

/* Brillo del backlight 0..100 %. */
void display_backlight_pct(uint8_t pct);

/* Inversion de color: la CYD (ILI9341) la necesitaba, este panel RGB no. Se
 * conserva la API (hmi_config/main.cpp la llaman igual) pero es un no-op en
 * este hardware -- solo guarda la bandera. */
void display_set_invert(bool on);
bool display_invert();
