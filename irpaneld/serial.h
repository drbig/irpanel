/** @file
 * Serial libray configuration
 *
 * @author Piotr S. Staszewski
 */

#ifndef IRPD_SERIAL
#define IRPD_SERIAL 1

// Configurable defines

#define SER_SEPARATORS  ":,"  //!< Separators for serial config string
#define SER_TIMEOUT     5     //!< Serial port read timeout in tenths of a second

// Public types and variables

typedef struct {
  int speed;
  int parity;
  int bitSize;
  int stopBits;
} SerialConfig;

extern SerialConfig serConfig;  //!< Global serial port config

// Public routines

void serial_parse(char *str);
void serial_setup(int fd);

#endif
