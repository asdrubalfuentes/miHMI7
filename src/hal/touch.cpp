#include "touch.h"
#include <lvgl.h>
#include <driver/i2c.h>
#include "config.h"
#include "display.h"

/* El GT911 queda inicializado solo: LGFX_Device::init() (llamado desde
 * display_hw_init(), ANTES que touch_hw_init()) ya hace
 * getPanel()->initTouch() internamente, sobre el mismo bus I2C
 * (I2C_NUM_1, ya instalado por LovyanGFX) que se usa para el escaneo de abajo. */

/* Escaneo de diagnostico: el tactil no respondia (reporte de banco, v0.1.0).
 * El GT911 puede levantar en 0x5D o 0x14 segun como quede polarizado su pin
 * INT durante el reset (que aqui no controlamos: TP_I2C_INT_PIN = -1 en
 * lgfx_panel.h) -- si el modulo real quedo en la otra direccion, GT911_I2C_ADDR
 * (config.h) no le pega a nada y por eso no responde. Este escaneo corre
 * sobre el bus que LovyanGFX ya dejo instalado (I2C_NUM_1) y dice, con datos
 * reales, que direcciones contestan -- para no seguir adivinando a ciegas. */
static void i2c_scan_log() {
	Serial.println("[touch] escaneo I2C (bus ya inicializado por LGFX)...");
	uint8_t found = 0;
	for (uint8_t addr = 1; addr < 127; addr++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (addr << 1) | I2C_MASTER_WRITE, true);
		i2c_master_stop(cmd);
		esp_err_t r = i2c_master_cmd_begin(I2C_NUM_1, cmd, pdMS_TO_TICKS(50));
		i2c_cmd_link_delete(cmd);
		if (r == ESP_OK) {
			Serial.printf("[touch]   responde 0x%02X%s\n", addr,
			              addr == GT911_I2C_ADDR ? "  <- GT911_I2C_ADDR configurado" : "");
			found++;
		}
	}
	if (!found) {
		Serial.println("[touch]   NADA respondio en el bus I2C -- revisar cableado SDA/SCL "
		                "(GPIO48/47), alimentacion del panel, o que el touch este soldado.");
	}
	Serial.println("[touch] fin del escaneo.");
}

void touch_hw_init() {
	i2c_scan_log();
}

void touch_calibrate_if_needed() { /* GT911: coordenadas de pixel ya calibradas de fabrica */ }
void touch_force_calibrate()     { /* idem -- boton "Recalibrar" de Ajustes queda sin efecto */ }

/* --- callback de lectura para LVGL ---
 * lgfx.getTouch() ya devuelve coordenadas de pixel en el espacio FISICO
 * 800x480 -- LVGL ahora dibuja nativo a esa misma resolucion (ver
 * hal/display.cpp v0.2.0), asi que el mapeo es 1:1, sin reescalar. */
static bool s_wasTouched = false;

static void touch_read(lv_indev_drv_t *drv, lv_indev_data_t *data) {
	(void)drv;
	int32_t x, y;
	bool touched = display_lgfx().getTouch(&x, &y);

	if (touched != s_wasTouched) {
		s_wasTouched = touched;
		Serial.println(touched ? "[touch] PRESS" : "[touch] release");
		if (touched) Serial.printf("[touch]   x=%d y=%d\n", (int)x, (int)y);
	}

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
