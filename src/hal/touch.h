/**
 * touch.h  -  Panel tactil capacitivo GT911 (I2C) + enlace con LVGL.
 *
 * A diferencia del XPT2046 resistivo de miHMI, el GT911 entrega coordenadas
 * de pixel ya calibradas de fabrica -- no hace falta rutina de calibracion.
 * touch_calibrate_if_needed() se conserva como no-op solo para que main.cpp
 * no necesite cambios (el boton "Recalibrar" de Ajustes, que llamaba a la
 * version "forzada", se quito de screen_settings.cpp por no tener efecto).
 */
#pragma once
#include <Arduino.h>

/* Arranca el tactil (comparte el bus I2C ya inicializado por display_hw_init/LGFX). */
void touch_hw_init();

/* No-op en este hardware (GT911 no necesita calibracion). */
void touch_calibrate_if_needed();

/* Registra el dispositivo de entrada en LVGL. Llamar tras lv_init(). */
void touch_lvgl_init();
