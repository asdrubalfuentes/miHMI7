#include "hmi_config.h"
#include <Preferences.h>

// Almacen de configuracion: NVS, con valores por defecto de config.h/secrets.h.
// Esta placa no tiene microSD (ver hmi_config.h) -- a diferencia de miHMI, no
// hay respaldo/espejo en tarjeta, todo vive en NVS directo.

namespace {

HmiConfig   g_cfg;
const char *g_src = "defaults";

Preferences g_nvs;

void cpstr(char *dst, const char *src, size_t n) {
	if (!src) { dst[0] = 0; return; }
	strncpy(dst, src, n - 1);
	dst[n - 1] = 0;
}

uint32_t fnv1a(const char *s) {
	uint32_t h = 2166136261u;
	for (; s && *s; ++s) { h ^= (uint8_t)*s; h *= 16777619u; }
	return h;
}

bool scaleVarEq(const ScaleVar &a, const ScaleVar &b) {
	return a.rawMin == b.rawMin && a.rawMax == b.rawMax && a.engMin == b.engMin
	    && a.engMax == b.engMax && a.unit   == b.unit   && a.filter == b.filter;
}

void apply_defaults() {
	cpstr(g_cfg.wifiSsid, WIFI_SSID, sizeof(g_cfg.wifiSsid));
	cpstr(g_cfg.wifiPass, WIFI_PASS, sizeof(g_cfg.wifiPass));
	cpstr(g_cfg.plcHost,  PLC_HOST,  sizeof(g_cfg.plcHost));
	g_cfg.plcPort = PLC_PORT;
	g_cfg.plcUnit = PLC_UNIT;
	g_cfg.pollMs  = MB_POLL_MS;
	g_cfg.pinHash = fnv1a(HMI_ADMIN_PIN_DEFAULT);
	g_cfg.theme   = 0;
	g_cfg.dispInvert = 0;      /* panel RGB565: sin necesidad de inversion (ver hal/display) */
	g_cfg.levelMaxM = 80;
	cpstr(g_cfg.ntpServer, "ntp.shoa.cl", sizeof(g_cfg.ntpServer));
	cpstr(g_cfg.tz, "<-04>4<-03>,M9.1.6/24,M4.1.6/24", sizeof(g_cfg.tz));  /* Chile continental */
	for (uint8_t s = 0; s < NUM_WELLS; s++)
		snprintf(g_cfg.stationName[s], WELL_NAME_LEN, "Estacion %u", (unsigned)(s + 1));

	g_cfg.light = hmicfg::lightDefaults();
}

// ---- NVS ----------------------------------------------------------------
void nvs_load() {
	g_nvs.begin("hmicfg", true);
	if (g_nvs.getBool("set", false)) {
		String ssid = g_nvs.getString("ssid", g_cfg.wifiSsid);
		String pass = g_nvs.getString("pass", g_cfg.wifiPass);
		String host = g_nvs.getString("host", g_cfg.plcHost);
		cpstr(g_cfg.wifiSsid, ssid.c_str(), sizeof(g_cfg.wifiSsid));
		cpstr(g_cfg.wifiPass, pass.c_str(), sizeof(g_cfg.wifiPass));
		cpstr(g_cfg.plcHost,  host.c_str(), sizeof(g_cfg.plcHost));
		{ String s = g_nvs.getString("wip",   g_cfg.wifiIp);   cpstr(g_cfg.wifiIp,   s.c_str(), sizeof(g_cfg.wifiIp)); }
		{ String s = g_nvs.getString("wgw",   g_cfg.wifiGw);   cpstr(g_cfg.wifiGw,   s.c_str(), sizeof(g_cfg.wifiGw)); }
		{ String s = g_nvs.getString("wmask", g_cfg.wifiMask); cpstr(g_cfg.wifiMask, s.c_str(), sizeof(g_cfg.wifiMask)); }
		{ String s = g_nvs.getString("wdns1", g_cfg.wifiDns1); cpstr(g_cfg.wifiDns1, s.c_str(), sizeof(g_cfg.wifiDns1)); }
		{ String s = g_nvs.getString("wdns2", g_cfg.wifiDns2); cpstr(g_cfg.wifiDns2, s.c_str(), sizeof(g_cfg.wifiDns2)); }
		g_cfg.plcPort = g_nvs.getUShort("port", g_cfg.plcPort);
		g_cfg.plcUnit = g_nvs.getUChar("unit", g_cfg.plcUnit);
		g_cfg.pollMs  = g_nvs.getUShort("poll", g_cfg.pollMs);
		g_cfg.pinHash = g_nvs.getULong("pin", g_cfg.pinHash);
		g_cfg.theme   = g_nvs.getUChar("theme", g_cfg.theme);
		g_cfg.dispInvert = g_nvs.getUChar("dinv", g_cfg.dispInvert);
		g_cfg.levelMaxM  = g_nvs.getUChar("lvlmax", g_cfg.levelMaxM);
		{ String s = g_nvs.getString("ntp", g_cfg.ntpServer); cpstr(g_cfg.ntpServer, s.c_str(), sizeof(g_cfg.ntpServer)); }
		{ String s = g_nvs.getString("tz",  g_cfg.tz);        cpstr(g_cfg.tz, s.c_str(), sizeof(g_cfg.tz)); }
		if (g_nvs.getBytesLength("light") == sizeof(g_cfg.light))    /* si no, se queda el default */
			g_nvs.getBytes("light", &g_cfg.light, sizeof(g_cfg.light));
		if (g_nvs.getBytesLength("scalec") == sizeof(g_cfg.scaleCache))
			g_nvs.getBytes("scalec", &g_cfg.scaleCache, sizeof(g_cfg.scaleCache));
		for (uint8_t s = 0; s < NUM_WELLS; s++) {
			char k[8]; snprintf(k, sizeof(k), "n%u", s);
			String n = g_nvs.getString(k, g_cfg.stationName[s]);
			cpstr(g_cfg.stationName[s], n.c_str(), WELL_NAME_LEN);
		}
		g_src = "NVS";
	}
	g_nvs.end();
}

void nvs_save() {
	g_nvs.begin("hmicfg", false);
	g_nvs.putString("ssid", g_cfg.wifiSsid);
	g_nvs.putString("pass", g_cfg.wifiPass);
	g_nvs.putString("host", g_cfg.plcHost);
	g_nvs.putString("wip",   g_cfg.wifiIp);
	g_nvs.putString("wgw",   g_cfg.wifiGw);
	g_nvs.putString("wmask", g_cfg.wifiMask);
	g_nvs.putString("wdns1", g_cfg.wifiDns1);
	g_nvs.putString("wdns2", g_cfg.wifiDns2);
	g_nvs.putUShort("port", g_cfg.plcPort);
	g_nvs.putUChar ("unit", g_cfg.plcUnit);
	g_nvs.putUShort("poll", g_cfg.pollMs);
	g_nvs.putULong ("pin",  g_cfg.pinHash);
	g_nvs.putUChar ("theme", g_cfg.theme);
	g_nvs.putUChar ("dinv",  g_cfg.dispInvert);
	g_nvs.putUChar ("lvlmax", g_cfg.levelMaxM);
	g_nvs.putString("ntp",   g_cfg.ntpServer);
	g_nvs.putString("tz",    g_cfg.tz);
	g_nvs.putBytes ("light", &g_cfg.light, sizeof(g_cfg.light));
	g_nvs.putBytes ("scalec", &g_cfg.scaleCache, sizeof(g_cfg.scaleCache));
	for (uint8_t s = 0; s < NUM_WELLS; s++) {
		char k[8]; snprintf(k, sizeof(k), "n%u", s);
		g_nvs.putString(k, g_cfg.stationName[s]);
	}
	g_nvs.putBool("set", true);
	g_nvs.end();
}

}  // namespace

namespace hmicfg {

bool begin(bool safeMode) {
	apply_defaults();

	if (safeMode) {                       /* MODO BASICO: solo defaults */
		g_src = "defaults (modo basico)";
		Serial.println("[cfg] MODO BASICO: NVS omitida, solo defaults");
		return true;
	}

	nvs_load();     /* deja g_src = "NVS" si habia algo guardado */
	Serial.printf("[cfg] origen=%s  ssid='%s'  plc=%s:%u u%u  poll=%u  pin=%s\n",
	              g_src, g_cfg.wifiSsid, g_cfg.plcHost, g_cfg.plcPort, g_cfg.plcUnit,
	              g_cfg.pollMs, g_cfg.pinHash ? "si" : "no");
	return true;
}

const HmiConfig &get()      { return g_cfg; }
HmiConfig        editable() { return g_cfg; }
bool             sdMounted(){ return false; }   /* sin microSD en esta placa */
const char      *source()   { return g_src; }

bool save(const HmiConfig &c) {
	g_cfg = c;
	nvs_save();
	Serial.println("[cfg] guardado en NVS");
	return true;
}

bool saveLight(const LightCfg &lc) {
	g_cfg.light = lc;
	nvs_save();
	Serial.println("[cfg] light guardado en NVS");
	return true;
}

bool saveTheme(uint8_t mode) {
	g_cfg.theme = mode;
	nvs_save();
	Serial.printf("[cfg] tema guardado (%u)\n", mode);
	return true;
}

bool saveDispInvert(uint8_t on) {
	g_cfg.dispInvert = on ? 1 : 0;
	nvs_save();
	Serial.printf("[cfg] inversion de panel guardada (%u)\n", g_cfg.dispInvert);
	return true;
}

bool saveLevelMaxM(uint8_t m) {
	g_cfg.levelMaxM = m ? m : 1;
	nvs_save();
	Serial.printf("[cfg] profundidad de escala guardada (%u m)\n", g_cfg.levelMaxM);
	return true;
}

bool saveScaleCache(uint8_t s, const StationScale &sc) {
	if (s >= NUM_WELLS) return false;
	ScaleCache &c = g_cfg.scaleCache[s];
	bool changed = !c.known || !scaleVarEq(c.level, sc.level) || !scaleVarEq(c.flow, sc.flow);
	if (!changed) return true;
	c.level = sc.level; c.flow = sc.flow; c.known = true;
	nvs_save();
	Serial.printf("[cfg] escala est.%u cacheada (nivel raw %u..%u)\n",
	              (unsigned)s, sc.level.rawMin, sc.level.rawMax);
	return true;
}

bool saveTime(const char *server, const char *tz) {
	if (server && *server) cpstr(g_cfg.ntpServer, server, sizeof(g_cfg.ntpServer));
	if (tz && *tz)         cpstr(g_cfg.tz, tz, sizeof(g_cfg.tz));
	nvs_save();
	Serial.printf("[cfg] NTP guardado: %s / %s\n", g_cfg.ntpServer, g_cfg.tz);
	return true;
}

LightCfg lightDefaults() {
	LightCfg d;
	d.enabled   = 0;       /* sin LDR de fabrica en esta placa: backlight fijo (blManual) */
	d.rawBright = 0;
	d.rawDark   = 300;
	d.pctClosed = 12;
	d.pctDay    = 80;
	d.pctTheme  = 55;
	d.hystPct   = 6;
	d.blClosed  = 0;
	d.blMin     = 20;
	d.blMax     = 100;
	d.blManual  = 80;
	return d;
}

uint32_t hashPin(const char *pin) { return fnv1a(pin); }

bool checkPin(const char *pin) {
	if (g_cfg.pinHash == 0) return true;          // sin PIN configurado
	return fnv1a(pin) == g_cfg.pinHash;
}

}  // namespace hmicfg
