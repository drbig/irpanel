#ifndef LCD_LIB
#define LCD_LIB 1

// Configurable defines

// ports and offset
#define LCD_DPORT   PORTB //!< Data port
#define LCD_CPORT   PORTB //!< Control port
#define LCD_DOFFSET 0x00  //!< Data pins' offset (0 - 4, on the data port)

// pins
#define LCD_RS      PB5   //!< Register select pin (on LCD_CPORT)
#define LCD_ENABLE  PB6   //!< Enable pin

/** LCD initialisation array.
 * This array should define 4 elements, composed from the appropriate LCD_CMD
 * entries combined with respective LCD_* arguments (if needed).
 */
#define LCD_INIT    {\
  (LCD_CMD_FUNC | LCD_FUNC_N),\
  (LCD_CMD_CTRL | LCD_CTRL_DISPLAY),\
  LCD_CMD_CLEAR,\
  (LCD_CMD_ENTRY | LCD_ENTRY_INC)}

// Public defines

// commands
#define LCD_CMD_CLEAR     0x01 //!< Clear display
#define LCD_CMD_HOME      0x02 //!< Move to home (0,0)
#define LCD_CMD_ENTRY     0x04 //!< Entry setup
#define LCD_CMD_CTRL      0x08 //!< Display control
#define LCD_CMD_CDSHIFT   0x10 //!< Display shift control
#define LCD_CMD_FUNC      0x20 //!< Dispaly function
#define LCD_CMD_CGRAM     0x40 //!< Set CGRAM address
#define LCD_CMD_DGRAM     0x80 //!< Set DGRAM address

// entry cmd args
#define LCD_ENTRY_SH      0x01 //!< If set, shift left, shift right otherwise
#define LCD_ENTRY_INC     0x02 //!< If set increment, decrement otherwise

// control cmd args
#define LCD_CTRL_BLINK    0x01 //!< Blink cursor
#define LCD_CTRL_CURSOR   0x02 //!< Show cursors
#define LCD_CTRL_DISPLAY  0x04 //!< Turn display on

// function cmd args
#define LCD_FUNC_F        0x04 //!< Display on
#define LCD_FUNC_N        0x08 //!< 2-line mode, otherwise 1-line
#define LCD_FUNC_DL       0x10 //!< 8-bit interface, otherwise 4-bit

// shift cmd args
#define LCD_CDSHIFT_RL    0x04 //!< Change shift to right-to-left

// Public routines

void lcd_send_byte(uint8_t data, bool chars, bool wait);
void lcd_init(void);
void lcd_goto(uint8_t x, uint8_t y);
void lcd_write_str(char *str);

#endif
