#include "display.h"
#include <lvgl.h>
#include <esp_heap_caps.h>
#include "config.h"

static LGFX g_lgfx;

/* Buffer parcial de LVGL, NATIVO 800x480 (sin sprite intermedio ni escalado
 * -- version anterior (sprite 320x240 + pushRotateZoom) se saco por temblor/
 * tearing visible en banco, ver CHANGELOG v0.2.0).
 *
 * En PSRAM y bien grande (DRAW_BUF_LINES): con un buffer chico LVGL llama a
 * disp_flush() muchas veces por cada cambio de pantalla, y cada pushImage()
 * al panel RGB tiene su propio costo fijo -- eso se sintio en banco como
 * "cambia muy lento entre pantallas". Menos llamadas, mas grandes cada una,
 * es la recomendacion estandar para paneles RGB de ESP32-S3 con LVGL. */
static lv_color_t *s_buf = nullptr;
static lv_disp_draw_buf_t s_draw_buf;
static lv_disp_drv_t s_disp_drv;

static uint8_t s_bl_pct  = 80;
static bool    s_invert  = false;

LGFX &display_lgfx() { return g_lgfx; }

void display_set_invert(bool on) {
	/* El panel RGB565 de esta placa no necesita (ni LovyanGFX/Panel_RGB
	 * expone) inversion de color -- era una particularidad del ILI9341 de la
	 * CYD. Se guarda la bandera para que hmi_config/main.cpp no cambien. */
	s_invert = on;
}
bool display_invert() { return s_invert; }

/* --- callback de volcado a pantalla ---
 * Escribe directo sobre el panel fisico (g_lgfx): sin sprite intermedio, sin
 * escalado. Cada rectangulo que LVGL termina de dibujar se manda tal cual.
 * (El log por-flush de diagnostico que corria aqui ya cumplio su proposito
 * en banco -- se quito en v0.2.2 por ser demasiado ruido en el puerto serie,
 * corria al menos una vez por segundo sin parar.) */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);

	/* setSwapBytes(true) en display_hw_init() corrige el orden de bytes de
	 * cada pixel (confirmado en banco: sin esto, texto/tarjetas salian con
	 * colores mezclados) -- lo hace LovyanGFX de una pasada al empujar,
	 * mas barato que el swap por pixel de LV_COLOR_16_SWAP en LVGL. */
	g_lgfx.pushImage(area->x1, area->y1, w, h, (uint16_t *)&color_p->full);

	lv_disp_flush_ready(drv);
}

void display_backlight_pct(uint8_t pct) {
	s_bl_pct = constrain(pct, 0, 100);
	g_lgfx.setBrightness(map(s_bl_pct, 0, 100, 0, 255));
}

void display_hw_init() {
	g_lgfx.init();
	g_lgfx.setSwapBytes(true);   /* ver nota en disp_flush() */
	g_lgfx.fillScreen(TFT_BLACK);
	display_backlight_pct(s_bl_pct);
}

void display_lvgl_init() {
	size_t bytes = (size_t)SCREEN_W * DRAW_BUF_LINES * sizeof(lv_color_t);
	s_buf = (lv_color_t *)heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM);
	if (!s_buf) {
		Serial.println("[display] sin PSRAM para el buffer de LVGL, cae a DRAM (mas chico)");
		s_buf = (lv_color_t *)malloc(bytes);
	}
	lv_disp_draw_buf_init(&s_draw_buf, s_buf, nullptr, (uint32_t)SCREEN_W * DRAW_BUF_LINES);

	lv_disp_drv_init(&s_disp_drv);
	s_disp_drv.hor_res  = SCREEN_W;
	s_disp_drv.ver_res  = SCREEN_H;
	s_disp_drv.flush_cb = disp_flush;
	s_disp_drv.draw_buf = &s_draw_buf;
	lv_disp_drv_register(&s_disp_drv);
}
