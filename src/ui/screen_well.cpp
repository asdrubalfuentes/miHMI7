#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include "data/hmi_config.h"
#include "hal/net_clock.h"
#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>

/* ---- widgets refrescados por screen_well_update() ---- */
static lv_obj_t *lbl_name;
static lv_obj_t *lbl_ticker;                 /* cintillo con tiras de mensajes */
static lv_obj_t *lbl_status;                 /* OK / ALARMA / SIN ENLACE */
static lv_obj_t *lbl_stnbtn;                 /* etiqueta del boton "Estacion N" */
static lv_obj_t *arc_level, *lbl_level_pct, *lbl_level_m;
static lv_obj_t *lbl_flow;
static lv_obj_t *lbl_today;
static lv_obj_t *lbl_alarms;

static uint8_t sel() { return DataHub::instance().selectedWell(); }
static const WellData &cur() { return DataHub::instance().data().well[sel()]; }

/* ------------------------- eventos ------------------------- */
static void on_actions(lv_event_t *e) { (void)e; ui_show_actions(); }

static void on_prev(lv_event_t *e) {
	(void)e;
	DataHub::instance().setSelectedWell((sel() + NUM_WELLS - 1) % NUM_WELLS);
	screen_well_update();
}
static void on_next(lv_event_t *e) {
	(void)e;
	DataHub::instance().setSelectedWell((sel() + 1) % NUM_WELLS);
	screen_well_update();
}
static void on_list(lv_event_t *e)     { (void)e; ui_show_wells();    }
static void on_hist(lv_event_t *e)     { (void)e; ui_show_history();  }
static void on_settings(lv_event_t *e) { (void)e; ui_show_settings(); }

/* ------------------------- helpers ------------------------- */
static lv_obj_t *icon_btn(lv_obj_t *parent, const char *sym, lv_coord_t w, lv_event_cb_t cb) {
	lv_obj_t *b = lv_btn_create(parent);
	lv_obj_set_size(b, w, 48);
	lv_obj_set_style_bg_color(b, COL_TEAL_D, 0);
	lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *l = lv_label_create(b);
	lv_label_set_text(l, sym);
	lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
	lv_obj_center(l);
	return b;
}

static lv_obj_t *action_btn(lv_obj_t *parent, const char *txt, lv_color_t col,
                            lv_coord_t x, lv_coord_t w, lv_event_cb_t cb) {
	lv_obj_t *b = lv_btn_create(parent);
	lv_obj_set_pos(b, x, SCREEN_H - 88);
	lv_obj_set_size(b, w, 70);
	lv_obj_set_style_bg_color(b, col, 0);
	lv_obj_set_style_radius(b, 10, 0);
	lv_obj_add_event_cb(b, cb, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *l = lv_label_create(b);
	lv_label_set_text(l, txt);
	lv_obj_set_style_text_font(l, &lv_font_montserrat_28, 0);
	lv_obj_center(l);
	return l;
}

/* ------------------------- create ------------------------- */
lv_obj_t *screen_well_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	ui_screen_bg(scr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	/* --- barra superior: volver | ‹ nombre › | ajustes --- */
	lv_obj_t *top = lv_obj_create(scr);
	lv_obj_set_pos(top, 0, 0);
	lv_obj_set_size(top, SCREEN_W, 60);
	lv_obj_set_style_bg_color(top, COL_CARD, 0);
	lv_obj_set_style_radius(top, 0, 0);
	lv_obj_set_style_border_width(top, 0, 0);
	lv_obj_set_style_pad_all(top, 0, 0);
	lv_obj_clear_flag(top, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *blist = icon_btn(top, LV_SYMBOL_LEFT, 68, on_list);
	lv_obj_align(blist, LV_ALIGN_LEFT_MID, 10, 0);

	lv_obj_t *bprev = icon_btn(top, LV_SYMBOL_LEFT, 52, on_prev);
	lv_obj_align(bprev, LV_ALIGN_CENTER, -150, 0);

	lbl_name = lv_label_create(top);
	lv_label_set_text(lbl_name, "Estacion");
	lv_obj_set_style_text_font(lbl_name, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_name, COL_TEXT, 0);
	lv_obj_align(lbl_name, LV_ALIGN_CENTER, 0, 0);

	lv_obj_t *bnext = icon_btn(top, LV_SYMBOL_RIGHT, 52, on_next);
	lv_obj_align(bnext, LV_ALIGN_CENTER, 150, 0);

	lv_obj_t *bset = icon_btn(top, LV_SYMBOL_SETTINGS, 68, on_settings);
	lv_obj_align(bset, LV_ALIGN_RIGHT_MID, -10, 0);

	/* --- cintillo: tira de mensajes con scroll circular --- */
	lv_obj_t *tk = lv_obj_create(scr);
	lv_obj_set_pos(tk, 0, 60);
	lv_obj_set_size(tk, SCREEN_W, 36);
	lv_obj_set_style_bg_color(tk, COL_SUNKEN, 0);
	lv_obj_set_style_radius(tk, 0, 0);
	lv_obj_set_style_border_width(tk, 0, 0);
	lv_obj_set_style_pad_all(tk, 0, 0);
	lv_obj_clear_flag(tk, LV_OBJ_FLAG_SCROLLABLE);

	/* Cintillo con scroll FORZADO (no LV_LABEL_LONG_SCROLL_CIRCULAR): ese modo
	 * solo anima si LVGL decide que el texto desborda, y en banco eso resulto
	 * poco confiable (no se movia, o se movia pero pegado a la izquierda con
	 * un ancho fijo que no coincidia con el contenido real). Ac aqui la
	 * etiqueta se autoajusta a su contenido (sin ancho fijo) y una animacion
	 * propia la desliza de derecha a izquierda sin parar, siempre -- "fijo
	 * aunque quepa", como se pidio. tk ya recorta lo que se sale de su caja
	 * (contenedor no-scrollable, comportamiento por defecto de LVGL). */
	lbl_ticker = lv_label_create(tk);
	lv_label_set_long_mode(lbl_ticker, LV_LABEL_LONG_CLIP);
	lv_label_set_text(lbl_ticker, APP_NAME);
	lv_obj_set_style_text_font(lbl_ticker, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(lbl_ticker, COL_MUTED, 0);
	lv_obj_align(lbl_ticker, LV_ALIGN_LEFT_MID, 0, 0);   /* fija Y; la animacion solo toca X */

	static lv_anim_t tickerAnim;
	lv_anim_init(&tickerAnim);
	lv_anim_set_var(&tickerAnim, lbl_ticker);
	lv_anim_set_exec_cb(&tickerAnim, (lv_anim_exec_xcb_t)lv_obj_set_x);
	lv_anim_set_values(&tickerAnim, SCREEN_W, -1800);   /* recorrido fijo, de sobra para el texto mas largo */
	lv_anim_set_time(&tickerAnim, 45000);                /* ~57 px/s, velocidad de lectura comoda */
	lv_anim_set_repeat_count(&tickerAnim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_start(&tickerAnim);

	/* --- nivel (arco) --- */
	arc_level = lv_arc_create(scr);
	lv_obj_set_size(arc_level, 250, 250);
	lv_obj_set_pos(arc_level, 40, 100);
	lv_arc_set_rotation(arc_level, 135);
	lv_arc_set_bg_angles(arc_level, 0, 270);
	lv_arc_set_range(arc_level, 0, 100);
	lv_arc_set_value(arc_level, 0);
	lv_obj_remove_style(arc_level, nullptr, LV_PART_KNOB);
	lv_obj_clear_flag(arc_level, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_set_style_arc_color(arc_level, COL_CARD, LV_PART_MAIN);
	lv_obj_set_style_arc_color(arc_level, COL_TEAL, LV_PART_INDICATOR);
	lv_obj_set_style_arc_width(arc_level, 26, LV_PART_MAIN);
	lv_obj_set_style_arc_width(arc_level, 26, LV_PART_INDICATOR);

	lbl_level_pct = lv_label_create(scr);
	lv_label_set_text(lbl_level_pct, "--%");
	lv_obj_set_style_text_font(lbl_level_pct, &lv_font_montserrat_48, 0);
	lv_obj_set_style_text_color(lbl_level_pct, COL_TEXT, 0);
	lv_obj_align_to(lbl_level_pct, arc_level, LV_ALIGN_CENTER, 0, -18);

	lbl_level_m = lv_label_create(scr);
	lv_label_set_text(lbl_level_m, "-- m");
	lv_obj_set_style_text_font(lbl_level_m, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(lbl_level_m, COL_MUTED, 0);
	lv_obj_align_to(lbl_level_m, arc_level, LV_ALIGN_CENTER, 0, 44);

	lv_obj_t *lvl_title = lv_label_create(scr);
	lv_label_set_text(lvl_title, "NIVEL");
	lv_obj_add_style(lvl_title, &st_title, 0);
	lv_obj_set_style_text_font(lvl_title, &lv_font_montserrat_20, 0);
	lv_obj_align_to(lvl_title, arc_level, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

	/* --- caudal + acumulado del dia --- */
	static const lv_coord_t RX = 420;   /* columna derecha */

	lv_obj_t *flow_title = lv_label_create(scr);
	lv_label_set_text(flow_title, "CAUDAL");
	lv_obj_add_style(flow_title, &st_title, 0);
	lv_obj_set_style_text_font(flow_title, &lv_font_montserrat_20, 0);
	lv_obj_set_pos(flow_title, RX, 104);

	lbl_flow = lv_label_create(scr);
	lv_label_set_text(lbl_flow, "-- L/s");
	lv_obj_set_style_text_font(lbl_flow, &lv_font_montserrat_48, 0);
	lv_obj_set_style_text_color(lbl_flow, COL_TEXT, 0);
	lv_obj_set_pos(lbl_flow, RX, 130);

	lv_obj_t *today_title = lv_label_create(scr);
	lv_label_set_text(today_title, "ACUMULADO HOY");
	lv_obj_add_style(today_title, &st_title, 0);
	lv_obj_set_style_text_font(today_title, &lv_font_montserrat_20, 0);
	lv_obj_set_pos(today_title, RX, 210);

	lbl_today = lv_label_create(scr);
	lv_label_set_text(lbl_today, "-- m3");
	lv_obj_set_style_text_font(lbl_today, &lv_font_montserrat_48, 0);
	lv_obj_set_style_text_color(lbl_today, COL_TEAL, 0);
	lv_obj_set_pos(lbl_today, RX, 236);

	/* estado de la estacion: bajo ACUMULADO (antes en el cintillo, se solapaba) */
	lbl_status = lv_label_create(scr);
	lv_label_set_text(lbl_status, "--");
	lv_obj_set_style_text_font(lbl_status, &lv_font_montserrat_28, 0);
	lv_obj_set_pos(lbl_status, RX, 310);

	/* alarma activa (linea que hace scroll) */
	lbl_alarms = lv_label_create(scr);
	lv_label_set_long_mode(lbl_alarms, LV_LABEL_LONG_SCROLL_CIRCULAR);
	lv_obj_set_width(lbl_alarms, SCREEN_W - RX - 20);
	lv_label_set_text(lbl_alarms, "");
	lv_obj_set_style_text_font(lbl_alarms, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(lbl_alarms, COL_WARN, 0);
	lv_obj_set_pos(lbl_alarms, RX, 350);

	/* --- botonera inferior: Estacion N | ACCIONES | historico --- */
	lbl_stnbtn = action_btn(scr, "Estacion", COL_TEAL_D, 20, 250, on_list);
	action_btn(scr, "ACCIONES", COL_TEAL_D, 280, 270, on_actions);
	action_btn(scr, LV_SYMBOL_LIST, COL_TEAL_D, 560, 210, on_hist);

	return scr;
}

/* ------------------------- update ------------------------- */
static const char *comm_str() {
	DataHub &hub = DataHub::instance();
	DataSource *pri = hub.primary();
	bool on_plc = pri && strcmp(hub.activeSourceName(), pri->name()) == 0;
	SrcHealth h = hub.primaryHealth();
	if (on_plc && h == SrcHealth::Ok)     return "PLC conectado";
	if (on_plc && h == SrcHealth::Stale)  return "PLC intermitente";
	return "PLC sin conexion (sim)";
}

static const char *first_alarm(uint16_t bits) {
	struct { uint16_t m; const char *n; } T[] = {
		{MAPB_ALM_LEVEL_HI, "Nivel alto"}, {MAPB_ALM_LEVEL_LOLO, "Marcha en seco"},
		{MAPB_ALM_LEVEL_LO, "Nivel bajo"}, {MAPB_ALM_NO_FLOW, "Sin caudal"},
		{MAPB_ALM_PRESS_FAIL, "Falla presostato"}, {MAPB_ALM_VOLT_LOSS, "Sin voltaje"},
		{MAPB_ALM_TAMPER, "Tapa abierta"}, {MAPB_ALM_LORA_LOSS, "Sin enlace LoRa"},
		{MAPB_ALM_STALE, "Dato obsoleto"}, {MAPB_ALM_SCALE_BAD, "Escala invalida"},
		{MAPB_ALM_OVERRANGE, "Sobre-rango"},
	};
	for (auto &e : T) if (bits & e.m) return e.n;
	return "";
}

void screen_well_update() {
	DataHub &hub = DataHub::instance();
	const WellData &d = hub.data().well[hub.selectedWell()];

	unsigned idx1 = (unsigned)(hub.selectedWell() + 1);
	lv_label_set_text_fmt(lbl_name, "%u/%u", idx1, (unsigned)NUM_WELLS);
	lv_label_set_text(lbl_stnbtn, d.name);

	/* cintillo: nombre app | estacion | version | hora | enlace | RSSI |
	   tramas OK/ERR | alarmas -- pedido en banco: mas datos de la estacion. */
	char hm[8]; netclock::hm(hm, sizeof(hm));
	char alm[48];
	if (d.inAlarm())        snprintf(alm, sizeof(alm), "ALARMA: %s", first_alarm(d.alarms));
	else if (d.hasLatched()) snprintf(alm, sizeof(alm), "pendiente: %s", first_alarm(d.alarmsLatched));
	else                      snprintf(alm, sizeof(alm), "sin alarmas");

	char tk[240];
	snprintf(tk, sizeof(tk),
	         "%s   |   %s   |   v%s   |   %s   |   %s   |   RSSI %d dBm   |   tramas %lu OK / %lu ERR   |   %s",
	         APP_NAME, d.name, APP_VERSION, hm, comm_str(),
	         d.rssi, (unsigned long)hub.txOk(), (unsigned long)hub.txErr(), alm);
	lv_label_set_text(lbl_ticker, tk);

	/* LVGL no formatea %f: usar snprintf de libc y set_text */
	char b[28];

	/* --- nivel: % grande (arco) + metros de profundidad debajo ---
	   Cambio de rumbo 2026-09: el nivel llega SIEMPRE ya escalado en metros
	 * (lo calibra el nodo remoto en su propio portal, no el HMI) -- ya no hay
	 * pagina de rangos ni bloque de escala que consultar aqui. La barra 0-100%
	 * es solo una referencia visual contra la profundidad de fondo de escala
	 * configurada (Ajustes, "profundidad a fondo de escala"). */
	float depthM = d.levelEng;   /* ya viene en metros, MB_LEVEL_SCALE ya aplicado */
	float hi = (float)hmicfg::get().levelMaxM;
	float pct = (hi > 0) ? (depthM / hi) * 100.0f : 0.0f;
	if (pct < 0) pct = 0; else if (pct > 100) pct = 100;

	lv_arc_set_value(arc_level, (int)lroundf(pct));
	snprintf(b, sizeof(b), "%d %%", (int)lroundf(pct));
	lv_label_set_text(lbl_level_pct, b);
	snprintf(b, sizeof(b), "%.2f m", depthM);
	lv_label_set_text(lbl_level_m, b);

	snprintf(b, sizeof(b), "%.1f m3/h", d.flowEng);   /* unidad fija, ver nodeIO ch[1].unit */
	lv_label_set_text(lbl_flow, b);
	snprintf(b, sizeof(b), "%.1f m3", d.totalDayM3);
	lv_label_set_text(lbl_today, b);

	if (!d.linkOk) {
		lv_label_set_text(lbl_status, "SIN ENLACE");
		lv_obj_set_style_text_color(lbl_status, COL_STOP, 0);
	} else if (d.inAlarm()) {
		lv_label_set_text(lbl_status, d.sirenOn ? "ALARMA*" : "ALARMA");
		lv_obj_set_style_text_color(lbl_status, COL_WARN, 0);
	} else if (d.hasLatched()) {
		lv_label_set_text(lbl_status, "PENDIENTE ACK");
		lv_obj_set_style_text_color(lbl_status, COL_WARN, 0);
	} else {
		lv_label_set_text(lbl_status, "OK");
		lv_obj_set_style_text_color(lbl_status, COL_RUN, 0);
	}

	lv_label_set_text(lbl_alarms,
	                  d.inAlarm()     ? first_alarm(d.alarms)
	                  : d.hasLatched() ? first_alarm(d.alarmsLatched)
	                                   : "");
}
