#ifndef RC5_LIB
#define RC5_LIB 1

// Configurable defines

// pins
#define RC5_PORT    PIND        //!< Input port
#define RC5_PIN     PD2         //!< Input pin

// interrupt
#define RC5_ISR     INT0_vect   //!< External interrupt vector
#define RC5_MCUCR   _BV(ISC00)  //!< Has to trigger on logic change
#define RC5_GIMSK   _BV(INT0)   //!< Which interrupt to use

// timer
#define RC5_TCNT    TCNT0       //!< Which timer to use
#define RC5_TCCR    TCCR0       //!< Control registers for that timer
#define RC5_TSCALE  _BV(CS02)   //!< Prescaler bits for the timer

// bit durations
#define RC5_SHORT_MIN 14        //!< 444 us
#define RC5_SHORT_MAX 42        //!< 1333 us
#define RC5_LONG_MIN  43        //!< 1334 us
#define RC5_LONG_MAX  69        //!< 2222 us

// Public defines

// getter macros
#define RC5_GetStartBits(cmd)           ((cmd & 0x3000) >> 12)
#define RC5_GetToggleBit(cmd)           ((cmd & 0x0800) >> 11)
#define RC5_GetAddressBits(cmd)         ((cmd & 0x07C0) >> 6)
#define RC5_GetCommandBits(cmd)         (cmd & 0x003F)
#define RC5_GetCommandAddressBits(cmd)  (cmd & 0x07FF)

// Public variables

extern volatile uint16_t rc5Cmd;  //!< Contains all bits received
extern volatile bool rc5HasCmd;   //!< Set true when full command has been received

// Public routines

void rc5_init(void);
void rc5_next(void);

// Interrupt handlers

ISR(RC5_ISR);

#endif
