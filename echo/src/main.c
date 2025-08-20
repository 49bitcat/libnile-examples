// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Adrian "asie" Siekierka, 2023
#include <wonderful.h>
#include <ws.h>
#include <wsx/console.h>
#include <nile.h>

__attribute__((section(".iramx_2bpp_3000")))
uint8_t tile_2bpp[WS_DISPLAY_TILE_SIZE * 256];
__attribute__((section(".iramx_1800")))
uint16_t screen[32 * 32];

void main(void) {
	// Initialize nileswan and MCU
	nile_io_unlock();
	nile_bank_unlock();
	nile_mcu_reset(false);

	ws_delay_ms(50);

	outportb(WS_SYSTEM_CTRL_COLOR_PORT, 0x00);
	nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

	wsx_console_init_default(screen);

	uint16_t last_held = 0;
	while (1) {
		// CDC read
		uint8_t c;
		int16_t result = nile_mcu_native_cdc_read_sync(&c, 1);
		if (result < 0) {
			outportb(WS_LCD_ICON_PORT, inportb(WS_LCD_ICON_PORT) + 1);
		} else if (result == 1) {
			wsx_console_putc(c);
		}

		// CDC write
		uint16_t curr_held = ws_keypad_scan();
		uint16_t keys_pressed = curr_held & ~last_held;
		last_held = curr_held;
		if (keys_pressed & WS_KEY_X1) nile_mcu_native_cdc_write_sync("X1", 2);
		if (keys_pressed & WS_KEY_X2) nile_mcu_native_cdc_write_sync("X2", 2);
		if (keys_pressed & WS_KEY_X3) nile_mcu_native_cdc_write_sync("X3", 2);
		if (keys_pressed & WS_KEY_X4) nile_mcu_native_cdc_write_sync("X4", 2);
		if (keys_pressed & WS_KEY_Y1) nile_mcu_native_cdc_write_sync("Y1", 2);
		if (keys_pressed & WS_KEY_Y2) nile_mcu_native_cdc_write_sync("Y2", 2);
		if (keys_pressed & WS_KEY_Y3) nile_mcu_native_cdc_write_sync("Y3", 2);
		if (keys_pressed & WS_KEY_Y4) nile_mcu_native_cdc_write_sync("Y4", 2);
		if (keys_pressed & WS_KEY_B) nile_mcu_native_cdc_write_sync("B", 1);
		if (keys_pressed & WS_KEY_A) nile_mcu_native_cdc_write_sync("A", 1);
		if (keys_pressed & WS_KEY_START) nile_mcu_native_cdc_write_sync("Start", 5);
	}
}
