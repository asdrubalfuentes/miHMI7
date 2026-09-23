#include "ota_hmi.h"
#include "net/ota_update.h"
#include "config.h"
#include "hal/display.h"
#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>

// Integracion del OTA en el HMI. Ver ota_hmi.h.
// Dibuja directo sobre el panel FISICO 800x480 (SCREEN_W/H, ver config.h),
// fuera del driver de LVGL -- pero SOLO dentro de un recuadro chico y fijo,
// centrado. El resto de la pantalla queda "congelado" tal cual lo dejo LVGL
// (loop() no vuelve a llamar lv_timer_handler() mientras ota::run() esta
// corriendo, ver otaHmi::service()/run_interactive() mas abajo) -- nunca se
// hace fillScreen() de toda la pantalla, ni aqui ni en los ticks de progreso.

namespace {

volatile bool  s_request   = false;
uint32_t       s_lastCheck = 0;
const uint32_t CHECK_EVERY_MS = 5UL * 60UL * 1000UL;   /* 5 min (antes 6 h) */

/* Recuadro chico y fijo, centrado -- geometria compartida por el redibujo de
 * fondo (mensaje) y por la barra de progreso dentro de el. */
static const int BOX_W = 420, BOX_H = 130;
static const int BOX_X = (SCREEN_W - BOX_W) / 2;
static const int BOX_Y = (SCREEN_H - BOX_H) / 2;
static const int BAR_X = BOX_X + 30, BAR_Y = BOX_Y + 58, BAR_W = BOX_W - 60, BAR_H = 50;

static void draw_pct(LGFX &t, int pct) {
	int fill = (BAR_W - 8) * (pct < 0 ? 0 : pct > 100 ? 100 : pct) / 100;
	/* Repinta el interior de la barra completo (vacio + relleno) antes del
	 * numero: asi el digito nuevo siempre tapa al anterior sin dejar
	 * fantasma, aunque tenga menos caracteres (ver nota de v0.2.3). */
	t.fillRect(BAR_X + 4, BAR_Y + 4, BAR_W - 8, BAR_H - 8, TFT_BLACK);
	t.fillRect(BAR_X + 4, BAR_Y + 4, fill, BAR_H - 8, TFT_GREEN);

	/* Numero DENTRO de la barra (pedido de banco), sin caja de fondo propia
	 * -- el interior ya se acaba de repintar entero arriba. */
	t.setTextDatum(MC_DATUM);
	t.setTextColor(TFT_WHITE);
	t.setTextSize(1);
	char b[8];
	snprintf(b, sizeof(b), "%d%%", pct);
	t.drawString(b, BAR_X + BAR_W / 2, BAR_Y + BAR_H / 2, 4);
}

void draw(ota::Phase ph, int pct, const char *d) {
	LGFX &t = display_lgfx();

	/* El progreso de descarga llama aqui una vez por cada punto de porcentaje
	 * (hasta 100 veces): el fondo (mensaje, marco de la barra) se pinta UNA
	 * sola vez al entrar a una fase nueva; cada tick de Download solo toca
	 * el interior de la barra + el numero -- un area chica, rapida de
	 * escribir incluso en un panel RGB sin doble buffer (ver v0.2.3). */
	static ota::Phase s_lastPhase = ota::Phase::Error;
	static bool       s_first     = true;
	bool newPhase = s_first || ph != s_lastPhase;
	s_first     = false;
	s_lastPhase = ph;

	if (newPhase) {
		t.fillRect(BOX_X, BOX_Y, BOX_W, BOX_H, TFT_BLACK);
		t.drawRect(BOX_X, BOX_Y, BOX_W, BOX_H, TFT_WHITE);

		t.setTextDatum(MC_DATUM);
		t.setTextColor(TFT_WHITE, TFT_BLACK);
		t.setTextSize(1);

		const char *m = "";
		switch (ph) {
			case ota::Phase::Check:    m = "Buscando actualizacion..."; break;
			case ota::Phase::UpToDate: m = "Firmware al dia";           break;
			case ota::Phase::Download: m = "Descargando firmware";      break;
			case ota::Phase::Verify:   m = "Verificando...";            break;
			case ota::Phase::Flash:    m = "Escribiendo...";            break;
			case ota::Phase::Done:     m = "Listo. Reiniciando";        break;
			case ota::Phase::Error:    m = "Error de actualizacion";    break;
		}
		/* Font 7 (usado antes de v0.2.1) es el set "7 segmentos" de
		 * LovyanGFX -- SOLO digitos, sin letras -- por eso el texto se
		 * dibujaba con el ancho mal calculado y se salia de pantalla
		 * (reporte de banco). Font 4 tiene alfabeto completo. */
		t.drawString(m, SCREEN_W / 2, BOX_Y + 26, 4);

		if (ph == ota::Phase::Download) {
			t.drawRect(BAR_X, BAR_Y, BAR_W, BAR_H, TFT_WHITE);
			draw_pct(t, pct);
		} else if (d && *d) {
			t.drawString(d, SCREEN_W / 2, BOX_Y + 74, 4);
		}
		return;
	}

	if (ph == ota::Phase::Download) draw_pct(t, pct);
}

void run_interactive() {
	ota::Config oc;
	oc.owner = OTA_GH_OWNER;
	oc.repo  = OTA_GH_REPO;
	oc.currentVersion = APP_VERSION;

	ota::Result r = ota::run(oc, draw);   // si actualiza, reinicia dentro
	if (r.ok && !r.hasUpdate)      { draw(ota::Phase::UpToDate, 0, "v" APP_VERSION); delay(1800); }
	else if (!r.ok)               { draw(ota::Phase::Error, 0, r.error);            delay(2600); }

	/* LVGL cree que su pantalla sigue dibujada: forzar un repintado completo
	 * (esto re-flusha el lienzo logico y lo vuelve a escalar al panel). */
	lv_obj_invalidate(lv_scr_act());
}

}  // namespace

namespace otaHmi {

void request() { s_request = true; }

void service() {
	if (WiFi.status() != WL_CONNECTED) return;

	if (s_request) {
		s_request   = false;
		s_lastCheck = millis();
		run_interactive();
		return;
	}

	/* el chequeo periodico espera a que el arranque se considere estable, para no
	 * pelear con el contador de reinicios de panic_screen. */
	if (millis() < BOOT_STABLE_MS + 5000UL) return;
	if (s_lastCheck != 0 && millis() - s_lastCheck < CHECK_EVERY_MS) return;
	s_lastCheck = millis();

	ota::Config oc;
	oc.owner = OTA_GH_OWNER;
	oc.repo  = OTA_GH_REPO;
	oc.currentVersion = APP_VERSION;
	ota::Result r = ota::check(oc, nullptr);    // silencioso
	if (r.ok && r.hasUpdate) run_interactive(); // hay version nueva -> aplica con pantalla
}

}  // namespace otaHmi
