/** @file
 * RC5 receiver driver
 * 
 * Code based on avr-rc5 library by Filip Sobalski, whic in turn was based
 * on the idea by Guy Carpenter. 
 * I have restructured the code a bit to fit with the other parts.
 *
 * @author Piotr S. Staszewski
 * @see https://github.com/pinkeen/avr-rc5
 * @see http://www.clearwater.com.au/rc5
 */

#include <stdint.h>
#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "rc5.h"

// Internal defines

// helpers
#define GLUE_NX(a, b) a##b          //!< Glue into symbol, no expansion
#define GLUE(a, b)    GLUE_NX(a, b) //!< Glue into symbol, with expansion

// registers
#define TCCRA GLUE(RC5_TCCR, A)
#define TCCRB GLUE(RC5_TCCR, B)

// Internal types and variables

typedef enum {
  STATE_START1, 
  STATE_MID1, 
  STATE_MID0, 
  STATE_START0, 
  STATE_ERROR, 
  STATE_BEGIN, 
  STATE_END
} StateEnum;

static const uint8_t TRANS[4] = {0x01, 0x91, 0x9b, 0xfb}; //!< Transition table
static uint8_t cnt;                                       //!< Bit position counter
static StateEnum state;                                   //!< Current state

// Public variables

volatile uint16_t rc5Cmd;
volatile bool rc5HasCmd;

// Public routines

/** Indicate ready to recieve next command.
 * One should *never* clear rc5HasCmd directly.
 *
 * @see rc5Cmd
 * @see rc5HasCmd
 */
void rc5_next() {
  rc5HasCmd = false;
  cnt = 14;
  rc5Cmd = 0;
  state = STATE_BEGIN;
  GIMSK |= RC5_GIMSK;
}

/** Initialise the timer and interrupts.
 * Setup prescaler and trigger for pin interrupt.
 */
void rc5_init() {
  MCUCR |= RC5_MCUCR;
  TCCRA = 0;
  TCCRB = RC5_TSCALE;
  rc5_next();
}

// Interrupt handlers

/** Pin logic-level change interrupt handler.
 * This is where all the bit processing happens.
 */
ISR(RC5_ISR) {
  uint8_t delay = RC5_TCNT;
  uint8_t event = bit_is_set(RC5_PORT, RC5_PIN) ? 2 : 0;

  if ((delay > RC5_LONG_MIN) && (delay < RC5_LONG_MAX))
    event += 4;
  else if ((delay < RC5_SHORT_MIN) || (delay > RC5_SHORT_MAX))
    rc5_next();

  if (state == STATE_BEGIN) {
    cnt--;
    rc5Cmd |= 1 << cnt;
    state = STATE_MID1;
    RC5_TCNT = 0;
    return;
  }

  StateEnum newstate = (TRANS[state] >> event) & 0x03;

  if ((newstate == state) || (state > STATE_START0)) {
    rc5_next();
    return;
  }

  state = newstate;

  if (state == STATE_MID0)
    cnt--;
  else if (state == STATE_MID1) {
    cnt--;
    rc5Cmd |= 1 << cnt;
  }

  if ((cnt == 0) && ((state == STATE_START1) || (state == STATE_MID0))) {
    state = STATE_END;
    rc5HasCmd = true;
    GIMSK &= ~RC5_GIMSK;
  }

  RC5_TCNT = 0;
}
