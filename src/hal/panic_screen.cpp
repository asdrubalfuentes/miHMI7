#include "panic_screen.h"
#include "display.h"
#include "config.h"
#include <Arduino.h>
#include <esp_system.h>
#include <Preferences.h>

// Aviso de fallo del HMI + modo basico tras reinicios en cadena.  Ver panic_screen.h.
//
// El motivo del reset se pide a esp_reset_reason() DESPUES de reiniciar: dibujar
// durante el panic real no es fiable (interrupciones cortadas, bus a medias), asi
// que el patron es panic -> reboot -> este modulo lo detecta y lo explica.
//
// Dibuja directo sobre el panel FISICO (800x480, display_lgfx()) -- corre
// antes de que LVGL este listo, asi que no pasa por el driver de LVGL.
//
// El contador de reinicios anormales va en NVS (flash): sobrevive a panic,
// watchdog, brownout y corte de alimentacion.  RTC_DATA_ATTR NO sirve: el
// arranque pone a cero .rtc.bss en cada reset.  La etapa de arranque es solo
// diagnostico y va en RTC_NOINIT_ATTR (mejor esfuerzo, sin desgaste de flash).

#define STAGE_MAGIC  0xB0075747u

RTC_NOINIT_ATTR static uint32_t s_stageMagic;
RTC_NOINIT_ATTR static char     s_stage[24];

static bool s_safeMode = false;

void panic_screen_stage(const char *name) {
	s_stageMagic = STAGE_MAGIC;
	strncpy(s_stage, name ? name : "?", sizeof(s_stage) - 1);
	s_stage[sizeof(s_stage) - 1] = 0;
}

bool panic_safe_mode() { return s_safeMode; }

/* --- contador de reinicios anormales en NVS --- */
static uint32_t fails_get() {
	Preferences p;
	p.begin("boot", true);
	uint32_t n = p.getUInt("fails", 0);
	p.end();
	return n;
}
static void fails_set(uint32_t n) {
	Preferences p;
	p.begin("boot", false);
	p.putUInt("fails", n);
	p.end();
}

void panic_boot_ok() {
	if (fails_get() != 0) {
		fails_set(0);
		Serial.println("[boot] arranque estable: contador de reinicios a 0");
	}
}

/* --- pintado comun: banda superior, titulo y hasta 3 lineas (pantalla FISICA) --- */
static void draw(uint16_t bg, const char *kicker, const char *title,
                 const char *l1, const char *l2, const char *l3) {
	LGFX &t = display_lgfx();

	display_backlight_pct(100);
	t.fillScreen(bg);
	t.setTextColor(TFT_WHITE, bg);

	t.setTextDatum(TL_DATUM);
	t.drawString(kicker, 24, 24, 4);
	t.drawFastHLine(0, 68, SCREEN_W, TFT_WHITE);

	t.setTextDatum(MC_DATUM);
	/* Font 7 (usado antes para titulos cortos) es el set "7 segmentos" de
	 * LovyanGFX -- solo digitos, sin letras -- rompia el ancho calculado y
	 * se salia de pantalla con texto real (mismo bug que en net/ota_hmi.cpp,
	 * encontrado ahi primero). Font 4 (alfabeto completo) + setTextSize
	 * para el mismo efecto de "mas grande en titulos cortos". */
	t.setTextSize(title && strlen(title) <= 18 ? 2 : 1);
	t.drawString(title, SCREEN_W / 2, 124, 4);
	t.setTextSize(1);

	t.setTextDatum(TL_DATUM);
	int16_t y = 200;
	if (l1) { t.drawString(l1, 24, y, 4); y += 48; }
	if (l2) { t.drawString(l2, 24, y, 4); y += 48; }
	if (l3) { t.drawString(l3, 24, y, 4); y += 48; }

	t.setTextDatum(BC_DATUM);
	t.setTextColor(TFT_YELLOW, bg);
	t.drawString(APP_NAME "  v" APP_VERSION, SCREEN_W / 2, SCREEN_H - 16, 4);
}

void panic_screen_fatal(const char *title, const char *l1,
                        const char *l2, const char *l3) {
	Serial.printf("[FATAL] %s | %s | %s | %s\n",
	              title ? title : "", l1 ? l1 : "", l2 ? l2 : "", l3 ? l3 : "");
	draw(TFT_RED, "FALLO FATAL - EQUIPO DETENIDO", title, l1, l2, l3);
	s_stageMagic = 0;                  /* no es un crash: no re-avisar al reiniciar */
	for (;;) delay(1000);
}

bool panic_screen_check_reset() {
	esp_reset_reason_t r = esp_reset_reason();

	if (r == ESP_RST_POWERON) {        /* encendido en frio: empezar limpio */
		fails_set(0);
		return false;
	}

	const char *why;
	const char *hint;
	switch (r) {
		case ESP_RST_PANIC:
			why = "Excepcion de software";
			hint = "Vea el backtrace en el monitor serie.";
			break;
		case ESP_RST_TASK_WDT:
			why = "Bloqueo (watchdog de tarea)";
			hint = "Un modulo no respondio > 5 s.";
			break;
		case ESP_RST_INT_WDT:
		case ESP_RST_WDT:
			why = "Watchdog";
			hint = "Vea el backtrace en el monitor serie.";
			break;
		case ESP_RST_BROWNOUT:
			why = "Alimentacion insuficiente";
			hint = "Cable USB corto o fuente 5V insuficiente.";
			break;
		default:
			return false;              /* reset por software, deep-sleep, etc. */
	}

	uint32_t fails = fails_get() + 1;
	fails_set(fails);

	char stage[40];
	if (s_stageMagic == STAGE_MAGIC && s_stage[0])
		snprintf(stage, sizeof(stage), "Se reinicio durante: %s", s_stage);
	else
		snprintf(stage, sizeof(stage), "Se reinicio durante: (desconocido)");

	Serial.printf("[RESET] motivo=%d (%s) | %s | fallos seguidos=%u/%u\n",
	              (int)r, why, stage, (unsigned)fails, BOOT_MAX_FAILS);

	if (fails >= BOOT_MAX_FAILS) {
		s_safeMode = true;
		Serial.println("[RESET] limite alcanzado -> MODO BASICO (defaults)");
		draw(TFT_NAVY, "MODO BASICO", "Config. por defecto",
		     why, stage, "Ajustes guardados: OMITIDOS");
		delay(7000);
		return true;
	}

	char line[40];
	snprintf(line, sizeof(line), "Reinicios seguidos: %u de %u",
	         (unsigned)fails, BOOT_MAX_FAILS);
	draw(TFT_RED, "EL HMI SE REINICIO SOLO", why, stage, hint, line);
	delay(7000);
	return true;
}
