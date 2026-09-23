#include "display.h"
#include <lvgl.h>
#include "config.h"

static LGFX         g_lgfx;
static LGFX_Sprite   canvas(&g_lgfx);   /* lienzo logico 320x240 en PSRAM; ver display_hw_init() */

/* Buffer parcial de LVGL (igual que miHMI: 1/6 de pantalla LOGICA aprox). */
static lv_color_t s_buf[SCREEN_W * DRAW_BUF_LINES];
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
 * LVGL dibuja en el lienzo logico 320x240 (canvas, en PSRAM); cuando termina
 * la pasada completa de refresco (lv_disp_flush_is_last), se escala TODO el
 * lienzo de una vez al panel fisico 800x480 con un solo pushRotateZoom --
 * mas barato que reescalar cada rectangulo parcial suelto. */
static void disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
	uint32_t w = (area->x2 - area->x1 + 1);
	uint32_t h = (area->y2 - area->y1 + 1);

	/* NOTA: si al probar en la placa real los colores salen con rojo/azul
	 * cambiados, agregar canvas.setSwapBytes(true) en display_hw_init() --
	 * no se pudo verificar visualmente sin el hardware en mano. */
	canvas.pushImage(area->x1, area->y1, w, h, (uint16_t *)&color_p->full);

	if (lv_disp_flush_is_last(drv))
		canvas.pushRotateZoom(&g_lgfx, 0.0f, UI_ZOOM_X, UI_ZOOM_Y);

	lv_disp_flush_ready(drv);
}

void display_backlight_pct(uint8_t pct) {
	s_bl_pct = constrain(pct, 0, 100);
	g_lgfx.setBrightness(map(s_bl_pct, 0, 100, 0, 255));
}

void display_hw_init() {
	g_lgfx.init();
	/* pushRotateZoom centra el lienzo escalado en el pivote del DESTINO;
	 * sin esto el pivote por defecto es (0,0) y el lienzo queda pegado a
	 * la esquina en vez de llenar el panel. */
	g_lgfx.setPivot(PHYS_SCREEN_W / 2.0f, PHYS_SCREEN_H / 2.0f);
	g_lgfx.fillScreen(TFT_BLACK);

	canvas.setColorDepth(16);
	canvas.setPsram(true);            /* 320*240*2 = 150 KB: va en PSRAM, no en el DRAM interno */
	canvas.createSprite(SCREEN_W, SCREEN_H);
	canvas.fillScreen(TFT_BLACK);

	display_backlight_pct(s_bl_pct);
}

void display_lvgl_init() {
	lv_disp_draw_buf_init(&s_draw_buf, s_buf, nullptr, SCREEN_W * DRAW_BUF_LINES);

	lv_disp_drv_init(&s_disp_drv);
	s_disp_drv.hor_res  = SCREEN_W;
	s_disp_drv.ver_res  = SCREEN_H;
	s_disp_drv.flush_cb = disp_flush;
	s_disp_drv.draw_buf = &s_draw_buf;
	lv_disp_drv_register(&s_disp_drv);
}
