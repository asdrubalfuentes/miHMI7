/**
 * lgfx_panel.h  -  Definicion LovyanGFX para el panel RGB565 800x480 + tactil
 * GT911 de la Panlee ZX7D00CE01SV13 (WT32-S3-WROVER).
 *
 * Pines y timings tomados del ejemplo oficial del fabricante (placa "SC05" en
 * su libreria, mismo panel que esta version usa):
 *   https://github.com/smartpanle/PanelLan_esp32_arduino/blob/master/src/board/sc05/sc05.cpp
 * Los valores de pines vienen de include/config.h (PIN_LCD_*, PIN_I2C_*) en
 * vez de estar sueltos aqui, para mantener un solo lugar de verdad con el
 * resto del proyecto.
 *
 * No hay expansor de IO (CH422G/TCA9554) en esta placa: LCD_RST y DISP_EN
 * simplemente no existen (el panel no los necesita).
 */
#pragma once
#define LGFX_USE_V1
#include <driver/i2c.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include "config.h"

class LGFX : public lgfx::LGFX_Device {
public:
	lgfx::Bus_RGB     _bus_instance;
	lgfx::Panel_RGB   _panel_instance;
	lgfx::Touch_GT911 _touch_instance;
	lgfx::Light_PWM   _light_instance;

	LGFX() {
		{
			auto cfg = _panel_instance.config_detail();
			// use_psram = 2 (auto) por defecto: esta placa tiene 8MB PSRAM, se usa sola.
			_panel_instance.config_detail(cfg);
		}
		{
			auto cfg   = _bus_instance.config();
			cfg.panel  = &_panel_instance;

			cfg.pin_d0  = PIN_LCD_D0;   // B0
			cfg.pin_d1  = PIN_LCD_D1;   // B1
			cfg.pin_d2  = PIN_LCD_D2;   // B2
			cfg.pin_d3  = PIN_LCD_D3;   // B3
			cfg.pin_d4  = PIN_LCD_D4;   // B4
			cfg.pin_d5  = PIN_LCD_D5;   // G0
			cfg.pin_d6  = PIN_LCD_D6;   // G1
			cfg.pin_d7  = PIN_LCD_D7;   // G2
			cfg.pin_d8  = PIN_LCD_D8;   // G3
			cfg.pin_d9  = PIN_LCD_D9;   // G4
			cfg.pin_d10 = PIN_LCD_D10;  // G5
			cfg.pin_d11 = PIN_LCD_D11;  // R0
			cfg.pin_d12 = PIN_LCD_D12;  // R1
			cfg.pin_d13 = PIN_LCD_D13;  // R2
			cfg.pin_d14 = PIN_LCD_D14;  // R3
			cfg.pin_d15 = PIN_LCD_D15;  // R4

			cfg.pin_pclk    = PIN_LCD_PCLK;
			cfg.pin_vsync   = PIN_LCD_VSYNC;
			cfg.pin_hsync   = PIN_LCD_HSYNC;
			cfg.pin_henable = PIN_LCD_DE;

			/* 16MHz (valor del fabricante) dio colores mezclados/morados en banco
			 * -- sintoma clasico de integridad de senal marginal en el cable/FPC
			 * del bus paralelo de 16 lineas. Se baja a 10MHz como primer intento
			 * mas conservador; si sigue mal, seguir bajando (8/6.5MHz) antes de
			 * sospechar de otra cosa (porches/polaridad). */
			cfg.freq_write = 10000000;

			cfg.hsync_polarity    = 1;
			cfg.hsync_front_porch = 20;
			cfg.hsync_pulse_width = 1;
			cfg.hsync_back_porch  = 87;
			cfg.vsync_polarity    = 1;
			cfg.vsync_front_porch = 5;
			cfg.vsync_pulse_width = 1;
			cfg.vsync_back_porch  = 31;
			cfg.pclk_active_neg   = 0;

			_bus_instance.config(cfg);
			_panel_instance.setBus(&_bus_instance);
		}
		{
			auto cfg = _panel_instance.config();

			cfg.memory_width  = SCREEN_W;
			cfg.panel_width   = SCREEN_W;
			cfg.memory_height = SCREEN_H;
			cfg.panel_height  = SCREEN_H;
			cfg.offset_x = 0;
			cfg.offset_y = 0;

			_panel_instance.config(cfg);
		}
		{
			auto cfg = _touch_instance.config();
			cfg.i2c_addr = GT911_I2C_ADDR;
			cfg.x_min = 0;
			cfg.x_max = SCREEN_W;
			cfg.y_min = 0;
			cfg.y_max = SCREEN_H;
			cfg.bus_shared      = false;
			cfg.offset_rotation = 0;
			cfg.i2c_port = I2C_NUM_1;
			cfg.pin_int  = -1;
			cfg.pin_sda  = PIN_I2C_SDA;
			cfg.pin_scl  = PIN_I2C_SCL;
			cfg.pin_rst  = -1;
			cfg.freq     = GT911_I2C_FREQ;
			_touch_instance.config(cfg);
			_panel_instance.setTouch(&_touch_instance);
		}
		{
			auto cfg = _light_instance.config();
			cfg.pin_bl      = PIN_LCD_BL;
			cfg.invert      = false;
			cfg.freq        = 22222;
			cfg.pwm_channel = 7;
			_light_instance.config(cfg);
			_panel_instance.setLight(&_light_instance);
		}
		setPanel(&_panel_instance);
	}
};
