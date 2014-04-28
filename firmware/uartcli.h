#ifndef UARTCLI_LIB
#define UARTCLI_LIB 1

// Configurable defines

#define CLI_BUFSIZ  24            //!< Buffer size (*max cmd length less one that this*).
#define CLI_BAUD    9600          //!< UART baud
#define CLI_ISR     USART_RX_vect //!< UART RX vector

// Public variables

extern volatile char cliBuffer[]; //!< Command data buffer
extern volatile bool cliHasCmd;   //!< Set to true when full command has been received

// Public routines

void uartcli_init(void);
void uartcli_next(void);
void uart_send_byte(uint8_t data);
void uart_write_str(char *str);

// Interrupt handlers

ISR(CLI_ISR);

#endif
