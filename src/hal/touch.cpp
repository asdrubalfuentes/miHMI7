#include "touch.h"
#include <lvgl.h>
#include "config.h"
#include "display.h"

/* El GT911 queda inicializado solo: LGFX_Device::init() (llamado desde
 * display_hw_init(), ANTES que touch_hw_init()) ya hace
 * getPanel()->initTouch() internamente -- no hace falta repetirlo aqui. */
void touch_hw_init() {
	Serial.println("[touch] GT911 listo (inicializado por LGFX::init())");
}

void touch_calibrate_if_needed() { /* GT911: coordenadas de pixel ya calibradas de fabrica */ }
void touch_force_calibrate()     { /* idem -- boton "Recalibrar" de Ajustes queda sin efecto */ }

/* --- callback de lectura para LVGL ---
 * lgfx.getTouch() devuelve coordenadas en el espacio FISICO 800x480 (segun
 * x_max/y_max configurados en lgfx_panel.h); se reescalan al espacio LOGICO
 * 320x240 que usa toda la UI (mismo factor que display.cpp usa al reves). */
static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
	(void)drv;
	int32_t px, py;
	if (!display_lgfx().getTouch(&px, &py)) {
		data->state = LV_INDEV_STATE_RELEASED;
		return;
	}
	int32_t x = (int32_t)(px / UI_ZOOM_X);
	int32_t y = (int32_t)(py / UI_ZOOM_Y);

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
