// SPDX-License-Identifier: CC0-1.0
//
// SPDX-FileContributor: Adrian "asie" Siekierka, 2026
#include <stdio.h>
#include <wonderful.h>
#include <ws.h>
#include <wsx/console.h>
#include <nile.h>

// #define DEBUG

__attribute__((section(".iramx_2bpp_3000")))
uint8_t tile_2bpp[WS_DISPLAY_TILE_SIZE * 256];
__attribute__((section(".iramx_1800")))
uint16_t screen[32 * 32];

#define RX_QUEUE_SIZE 2048
#define RX_QUEUE_MASK ((RX_QUEUE_SIZE) - 1)
uint8_t rx_queue[RX_QUEUE_SIZE];
uint16_t rx_queue_read = 0;
uint16_t rx_queue_write = 0;

__attribute__((assume_ss_data, interrupt))
void uart_rx_handler(void) {
	rx_queue[rx_queue_write] = inportb(WS_UART_DATA_PORT);
	rx_queue_write = (rx_queue_write + 1) & RX_QUEUE_MASK;
	ws_int_ack(WS_INT_ACK_UART_RX);
}

void main(void) {
	// Initialize nileswan and MCU
	nile_io_unlock();
	nile_bank_unlock();
	nile_mcu_reset(false);

	ws_delay_ms(50);

	outportb(WS_SYSTEM_CTRL_COLOR_PORT, 0x00);
	nile_spi_set_control(NILE_SPI_CLOCK_CART | NILE_SPI_DEV_MCU);

	wsx_console_init_default(screen);

	printf("Y1/Y2/Y3 to control UART speed\n");

	ws_int_set_handler(WS_INT_UART_RX, (ia16_int_handler_t) uart_rx_handler);
	ws_int_set_enabled(WS_INT_ENABLE_UART_RX);
	ia16_enable_irq();

	outportb(0xA3, 0);
	ws_uart_open(WS_UART_BAUD_RATE_9600);

	uint16_t last_held = 0;
	while (1) {
		// CDC -> UART
		uint8_t c;
		int16_t result = nile_mcu_native_cdc_read_sync(&c, 1);
		if (result < 0) {
			outportb(WS_LCD_ICON_PORT, inportb(WS_LCD_ICON_PORT) + 1);
		} else if (result == 1) {
			ws_uart_putc(c);
#ifdef DEBUG
			printf("v %02X\n", c);
#endif
		}

		// UART -> CDC
		if (rx_queue_write != rx_queue_read) {
			uint16_t to_read = 0;
			if (rx_queue_write > rx_queue_read) {
				to_read = rx_queue_write - rx_queue_read;
			} else {
				to_read = RX_QUEUE_SIZE - rx_queue_read;
			}
#ifdef DEBUG
			printf("^ %d\n", to_read);
#endif
			if (to_read > 128) to_read = 128;
			int16_t read = nile_mcu_native_cdc_write_sync(rx_queue + rx_queue_read, to_read);
			if (read > 0) {
				rx_queue_read = (rx_queue_read + read) & RX_QUEUE_MASK;
			}
		}

		// control
		uint16_t curr_held = ws_keypad_scan();
		uint16_t keys_pressed = curr_held & ~last_held;
		last_held = curr_held;
		if (keys_pressed & WS_KEY_Y1) {
		        outportb(0xA3, 0);
		        ws_uart_open(WS_UART_BAUD_RATE_9600);
		        printf("UART speed: 9600 bps\n");
		}
		if (keys_pressed & WS_KEY_Y2) {
		        outportb(0xA3, 0);
		        ws_uart_open(WS_UART_BAUD_RATE_38400);
		        printf("UART speed: 38400 bps\n");
		}
		if (keys_pressed & WS_KEY_Y3) {
		        outportb(0xA3, 8);
		        ws_uart_open(WS_UART_BAUD_RATE_38400);
		        printf("UART speed: 192000 bps\n");
		}
	}
}
