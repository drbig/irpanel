/** @file
 * HD44780-based 4-bit LCD driver
 *
 * Minimal LCD driver (4-bit, 6-pin).
 *
 * @author Piotr S. Staszewski
 */

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <util/delay.h>

#include "lcd.h"

// Internal defines

#define SET(port, bit) port |= _BV(bit)
#define CLR(port, bit) port &= ~_BV(bit)

// Internal data

static const uint8_t LCD_INIT_ARY[4] = LCD_INIT;

// Internal functions

/** Send nibble to lcd.
 *
 *  @param data The nibble to send
 *  @private
 */
static void lcd_send_nibble(uint8_t data) {
  SET(LCD_CPORT, LCD_ENABLE);
  LCD_DPORT &= ~(0x0f << LCD_DOFFSET);
  LCD_DPORT |= (data << LCD_DOFFSET);
  __builtin_avr_delay_cycles(1);
  CLR(LCD_CPORT, LCD_ENABLE);
  __builtin_avr_delay_cycles(5);
}

// Public functions

/** Send raw byte to lcd.
 *
 * @param data  The byte to send
 * @param chars True if sending character, false if sending command
 * @param wait  If true wait additional 5 ms
 */
void lcd_send_byte(uint8_t data, bool chars, bool wait) {
  if (chars) SET(LCD_CPORT, LCD_RS);
  lcd_send_nibble(data >> 4);
  lcd_send_nibble(data & 0x0f);
  if (chars) CLR(LCD_CPORT, LCD_RS);
  _delay_ms(1);
  if (wait) _delay_ms(5);
}

/** Initialise the lcd.
 * Send the 'standard' 3x 0x03 header and then the true init packets.
 *
 * @see LCD_INIT
 */
void lcd_init() {
  uint8_t i;

  for (i = 0; i < 3; i++) {
    lcd_send_nibble(0x03);
    _delay_ms(1);
  }
  lcd_send_nibble(0x02);
  _delay_ms(1);

  for (i = 0; i < 4; i++)
    lcd_send_byte(LCD_INIT_ARY[i], false, true);
}

/** Go to specific position.
 * Both positions are 0 indexed. No input checking is done whatsoever.
 *
 * @param x The horizontal coordinate
 * @param y The vertical coordinate
 */
void lcd_goto(uint8_t x, uint8_t y) {
  uint8_t addr = 0;

  switch (y) {
    case 0: addr = 0x80; break;
    case 1: addr = 0xc0; break;
    case 2: addr = 0x94; break;
    case 3: addr = 0xd4; break;
  }
  addr += x;
  lcd_send_byte(addr, false, false);
}

/** Write character string to lcd.
 *
 * @param str The pointer to the string to write
 */
void lcd_write_str(char *str) {
  while (*str) lcd_send_byte((uint8_t) *str++, true, false);
}
