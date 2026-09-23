#include "touch.h"
#include <lvgl.h>
#include "config.h"
#include "display.h"

/* El GT911 queda inicializado solo: LGFX_Device::init() (llamado desde
 * display_hw_init(), ANTES que touch_hw_init()) ya hace
 * getPanel()->initTouch() internamente, sobre el mismo bus I2C (I2C_NUM_1,
 * ya instalado por LovyanGFX). Confirmado en banco (responde en
 * GT911_I2C_ADDR, ver config.h) -- el escaneo I2C de diagnostico que corria
 * aqui en cada arranque ya cumplio su proposito y se quito (v0.2.2, pedido
 * de banco: menos ruido por el puerto serie). */

void touch_hw_init() {}

void touch_calibrate_if_needed() { /* GT911: coordenadas de pixel ya calibradas de fabrica */ }
void touch_force_calibrate()     { /* idem -- boton "Recalibrar" de Ajustes queda sin efecto */ }

/* --- callback de lectura para LVGL ---
 * lgfx.getTouch() ya devuelve coordenadas de pixel en el espacio FISICO
 * 800x480 -- LVGL ahora dibuja nativo a esa misma resolucion (ver
 * hal/display.cpp v0.2.0), asi que el mapeo es 1:1, sin reescalar. */
static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
	(void)drv;
	int32_t x, y;
	bool touched = display_lgfx().getTouch(&x, &y);

	if (!touched) {
		data->state = LV_INDEV_STATE_RELEASED;
		return;
	}
	data->point.x = (lv_coord_t)constrain(x, 0, SCREEN_W - 1);
	data->point.y = (lv_coord_t)constrain(y, 0, SCREEN_H - 1);
	data->state   = LV_INDEV_STATE_PRESSED;
}

void touch_lvgl_init() {
	static lv_indev_drv_t indev_drv;
	lv_indev_drv_init(&indev_drv);
	indev_drv.type    = LV_INDEV_TYPE_POINTER;
	indev_drv.read_cb = touch_read;
	lv_indev_drv_register(&indev_drv);
}
