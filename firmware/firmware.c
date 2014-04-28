/** @file
 * IRPanel firmware
 *
 * Tie together all parts and provide a simple packet-based CLI
 * via UART.
 *
 * @author Piotr S. Staszewski
 */

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include "lcd.h"
#include "rc5.h"
#include "uartcli.h"

// Configurable defines

#define PWM_INITIAL 0x80  //!< Initial value for PWM

// Main routine

int main() {
  DDRA = 0xff;
  DDRB = 0xff;
  DDRD = 0x72;

  PORTA = 0x00;
  PORTB = 0x00;

  // Fast 8-bit PWM on OC1B (PD4), 1/8 prescaler
  TCCR1A = _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS11);
  OCR1B = PWM_INITIAL;

  uartcli_init();
  rc5_init();
  lcd_init();

  sei();

  while (true) {
    sleep_mode(); // enter idle mode
    if (rc5HasCmd) {
      cli();
      uart_send_byte(0x03);         // packet length
      uart_send_byte((uint8_t)'i'); // input code
      uart_send_byte((uint8_t)RC5_GetAddressBits(rc5Cmd));
      uart_send_byte((uint8_t)RC5_GetCommandBits(rc5Cmd));
      rc5_next();
      sei();
    }
    if (cliHasCmd) {
      cli();
      switch (cliBuffer[0]) {
        case 'c': // clear lcd
          lcd_send_byte(LCD_CMD_CLEAR, false, true);
          break;
        case 'd': // dim lcd (set PWM TOP)
          OCR1B = (uint8_t)cliBuffer[1];
          break;
        case 'h': // home lcd
          lcd_send_byte(LCD_CMD_HOME, false, true);
          break;
        case 'g': // move cursor to x,y
          lcd_goto((uint8_t)cliBuffer[1], (uint8_t)cliBuffer[2]);
          break;
        case 'p': // print character string
          lcd_write_str((char *)cliBuffer + 1);
          break;
        case 'r': // send raw byte to the lcd
          lcd_send_byte((uint8_t)cliBuffer[1], (bool)cliBuffer[2], (bool)cliBuffer[3]);
          break;
        default: // output received data, for testing
          #ifdef DEBUG
            uart_write_str((char *)cliBuffer);
          #endif
          break;
      }
      uart_send_byte(0x01);         // packet length
      uart_send_byte((uint8_t)'d'); // done code
      uartcli_next();
      sei();
    }
  }
}
