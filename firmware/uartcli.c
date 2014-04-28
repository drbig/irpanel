/** @file
 * UART packet-based driver
 *
 * Recieve commands as simple byte-oriented packets. The first byte indicates the
 * length of the command to receieve, rest is the payload.
 *
 * Can be trivially extended with address byte as the first byte to make a simple
 * bus protocol that can handle upto 256 devices.
 *
 * @author Piotr S. Staszewski
 */

#include <stdint.h>
#include <stdbool.h>

#include <avr/interrupt.h>
#include <util/delay.h>

#include "uartcli.h"

// Internal defines

#define CLI_PRESCALE  (((F_CPU / (16UL * CLI_BAUD))) - 1) //!< UART prescaler

// Internal variables

static volatile uint8_t cmdLen = 0; //!< Command length
static volatile uint8_t cmdPtr = 0; //!< Pointer to current byte

// Public variables

volatile char cliBuffer[CLI_BUFSIZ];
volatile bool cliHasCmd = false;

// Public routines

/** Initialise the UART CLI.
 * Will setup the UART.
 */
void uartcli_init() {
  UCSRB = _BV(RXEN) | _BV(TXEN) | _BV(RXCIE);
  UCSRC = _BV(UCSZ0) | _BV(UCSZ1);
  UBRRH = (CLI_PRESCALE >> 8);
  UBRRL = CLI_PRESCALE;
}

/** Indicated ready to receive next command.
 * Has to be called after a command has been processed to enable
 * further processing. One should *never* clear cliHasCmd directly.
 *
 * @see cliBuffer
 * @see cliHasCmd
 */
void uartcli_next() {
  cliHasCmd = false;
  cmdLen = 0;
  cmdPtr = 0;
  UCSRB |= _BV(RXCIE);
}

/** Send signle byte over UART.
 * @param data The byte to send
 */
void uart_send_byte(uint8_t data) {
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = data;
}

/** Send character string over UART.
 *
 * @param str Pointer to the string to send
 */
void uart_write_str(char *str) {
  while (*str) uart_send_byte((uint8_t)*str++);
}

// Interrupt handlers

/** UART Recieve interrupt handler.
 */
ISR(CLI_ISR) {
  if (cmdLen > cmdPtr) {
    cliBuffer[cmdPtr] = UDR;
    cmdPtr++;
    if (cmdLen == cmdPtr) {
      cliBuffer[cmdPtr] = 0;
      cliHasCmd = true;
      UCSRB &= ~_BV(RXCIE);
    }
  } else
    cmdLen = UDR;
}
