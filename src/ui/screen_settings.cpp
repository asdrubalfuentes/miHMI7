#include "ui/ui.h"
#include "ui/ui_screens.h"
#include "ui/theme.h"
#include "config.h"
#include "data/data_hub.h"
#include "data/hmi_config.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <WiFi.h>

static lv_obj_t *lbl_plc;       /* estado del enlace con el PLC (grande) */
static lv_obj_t *lbl_ip;        /* IP del HMI */
static lv_obj_t *lbl_wifi;      /* SSID + RSSI */
static lv_obj_t *lbl_origin;
static lv_obj_t *lbl_counters;

static void on_back(lv_event_t *e) { (void)e; ui_show_wells(); }
static void on_config(lv_event_t *e) { (void)e; ui_show_pin(ui_show_config); }

static lv_obj_t *row(lv_obj_t *parent, const char *k, const char *v, lv_coord_t y) {
	lv_obj_t *lk = lv_label_create(parent);
	lv_label_set_text(lk, k);
	lv_obj_set_style_text_font(lk, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(lk, COL_MUTED, 0);
	lv_obj_align(lk, LV_ALIGN_TOP_LEFT, 24, y);

	lv_obj_t *lv = lv_label_create(parent);
	lv_label_set_text(lv, v);
	lv_obj_set_style_text_font(lv, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(lv, COL_TEXT, 0);
	lv_obj_align(lv, LV_ALIGN_TOP_LEFT, 320, y);
	return lv;
}

lv_obj_t *screen_settings_create() {
	lv_obj_t *scr = lv_obj_create(nullptr);
	ui_screen_bg(scr);
	lv_obj_set_style_bg_color(scr, COL_BG, 0);
	lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
	lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *title = lv_label_create(scr);
	lv_label_set_text(title, "Ajustes / Diagnostico");
	lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
	lv_obj_set_style_text_color(title, COL_TEXT, 0);
	lv_obj_align(title, LV_ALIGN_TOP_LEFT, 20, 16);

	/* Volver: esquina superior derecha (antes abajo, se solapaba con Escala/Recalibr.) */
	lv_obj_t *back = lv_btn_create(scr);
	lv_obj_set_size(back, 160, 56);
	lv_obj_align(back, LV_ALIGN_TOP_RIGHT, -20, 10);
	lv_obj_set_style_bg_color(back, COL_TEAL_D, 0);
	lv_obj_add_event_cb(back, on_back, LV_EVENT_CLICKED, nullptr);
	lv_obj_t *bl = lv_label_create(back);
	lv_label_set_text(bl, LV_SYMBOL_LEFT " Volver");
	lv_obj_set_style_text_font(bl, &lv_font_montserrat_20, 0);
	lv_obj_center(bl);

	/* estado del enlace con el PLC: la respuesta a "estoy conectado?" */
	lv_obj_t *k = lv_label_create(scr);
	lv_label_set_text(k, "Enlace PLC");
	lv_obj_set_style_text_font(k, &lv_font_montserrat_20, 0);
	lv_obj_set_style_text_color(k, COL_MUTED, 0);
	lv_obj_align(k, LV_ALIGN_TOP_LEFT, 24, 92);
	lbl_plc = lv_label_create(scr);
	lv_label_set_text(lbl_plc, "--");
	lv_obj_set_style_text_font(lbl_plc, &lv_font_montserrat_28, 0);
	lv_obj_align(lbl_plc, LV_ALIGN_TOP_LEFT, 320, 86);

	const HmiConfig &c = hmicfg::get();
	char buf[56];
	snprintf(buf, sizeof(buf), "%s:%u  u%u", c.plcHost, c.plcPort, c.plcUnit);
	row(scr, "PLC destino", buf, 144);
	lbl_ip     = row(scr, "IP del HMI", "--", 186);
	lbl_wifi   = row(scr, "WiFi", c.wifiSsid[0] ? c.wifiSsid : "sin configurar", 228);
	lbl_origin = row(scr, "Origen / latido", "--", 270);
	lbl_counters = row(scr, "Tramas OK / ERR", "0 / 0", 312);
	snprintf(buf, sizeof(buf), "%s%s", hmicfg::source(),
	         hmicfg::sdMounted() ? " (microSD OK)" : " (sin microSD)");
	row(scr, "Config", buf, 354);

	/* El boton "Escala" se quito: la calibracion ahora vive en el portal del
	 * nodo remoto (cambio de rumbo 2026-09), no en el HMI. El boton
	 * "Recalibr." tambien se quito: el GT911 es tactil capacitivo, calibrado
	 * de fabrica -- no tiene rutina de recalibracion (ver hal/touch.h), asi
	 * que el boton no hacia nada (reporte de banco). */
	lv_obj_t *bcfg = lv_btn_create(scr);
	lv_obj_set_size(bcfg, 260, 64);
	lv_obj_align(bcfg, LV_ALIGN_TOP_LEFT, 24, 404);
	lv_obj_set_style_bg_color(bcfg, COL_ORANGE, 0);
	lv_obj_add_event_cb(bcfg, on_config, LV_EVENT_CLICKED, nullptr);
	lv_obj_center(lv_label_create(bcfg));
	lv_label_set_text(lv_obj_get_child(bcfg, 0), "Configuracion");
	lv_obj_set_style_text_font(lv_obj_get_child(bcfg, 0), &lv_font_montserrat_20, 0);

	/* debajo del titulo: hay hueco antes de la primera fila (y=92) */
	lv_obj_t *ver = lv_label_create(scr);
	lv_label_set_text_fmt(ver, "%s  v%s", APP_NAME, APP_VERSION);
	lv_obj_add_style(ver, &st_title, 0);
	lv_obj_set_style_text_font(ver, &lv_font_montserrat_16, 0);
	lv_obj_align(ver, LV_ALIGN_TOP_LEFT, 24, 58);

	return scr;
}

void screen_settings_update() {
	DataHub &hub = DataHub::instance();
	const PlantData &p = hub.data();
	DataSource *pri = hub.primary();

	bool on_plc = pri && strcmp(hub.activeSourceName(), pri->name()) == 0;
	SrcHealth h = hub.primaryHealth();

	const char *txt; lv_color_t col;
	if (on_plc && h == SrcHealth::Ok)        { txt = "CONECTADO";           col = COL_RUN;  }
	else if (on_plc && h == SrcHealth::Stale){ txt = "INTERMITENTE";        col = COL_WARN; }
	else                                     { txt = "SIN CONEXION (sim)";  col = COL_STOP; }
	lv_label_set_text(lbl_plc, txt);
	lv_obj_set_style_text_color(lbl_plc, col, 0);

	/* IP/RSSI: WiFi es del equipo, no de la fuente de datos activa -- antes se
	 * pedian a hub.primary() (DataSource::localIp()/linkRssi()), pero solo
	 * ModbusTcpSource los implementa de verdad; con MockSource de primaria
	 * (banco de pruebas) esos campos quedaban en "-"/0 pese a estar
	 * conectado. WiFi.* refleja el enlace real sin importar cual fuente esta
	 * activa. */
	bool wifiUp = WiFi.status() == WL_CONNECTED;
	const char *ssid = hmicfg::get().wifiSsid;
	if (wifiUp) lv_label_set_text_fmt(lbl_ip, "%s", WiFi.localIP().toString().c_str());
	else        lv_label_set_text(lbl_ip, "sin WiFi");
	if (wifiUp) lv_label_set_text_fmt(lbl_wifi, "%s  %d dBm", ssid[0] ? ssid : "--", (int)WiFi.RSSI());
	else        lv_label_set_text_fmt(lbl_wifi, "%s  desconectado", ssid[0] ? ssid : "--");
	lv_label_set_text_fmt(lbl_origin, "%s  ~ %u",
	                      p.origin ? "LOGO! real" : "PLC-SIM", p.heartbeat);
	lv_label_set_text_fmt(lbl_counters, "%lu / %lu",
	                      (unsigned long)hub.txOk(), (unsigned long)hub.txErr());
}
