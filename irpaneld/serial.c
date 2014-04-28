/** @file
 * Serial library
 *
 * Basic code to setup the serial port.
 *
 * @author Piotr S. Staszewski
 */

#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "common.h"
#include "serial.h"

// Public variables

SerialConfig serConfig;

// Public routines

/** Parse serial port mode string.
 * The format its 'speedXparityXbitSizeXstopBits' where X is either
 * a ':' or ',' (a separator). Currently code only for most common values.
 * Will either fully succeed or die.
 *
 * @param str Pointer to string containing serial mode
 */
void serial_parse(char *str) {
  char *token = NULL;
  int position, temp;

  position = temp = 0;
  bzero(&serConfig, sizeof(serConfig));

  token = strtok(str, SER_SEPARATORS); 
  while (token != NULL) {
    switch (position) {
      case 0: // speed
        temp = atoi(token);
        switch (temp) {
          case 115200:  serConfig.speed = B115200; break;
          case 57600:   serConfig.speed = B57600;  break;
          case 38400:   serConfig.speed = B38400;  break;
          case 19200:   serConfig.speed = B19200;  break;
          case 9600:    serConfig.speed = B9600;   break;
          case 4800:    serConfig.speed = B4800;   break;
          case 2400:    serConfig.speed = B2400;   break;
          case 1200:    serConfig.speed = B1200;   break;
          default:
            die("Unrecognised serial speed");
            break;
        }
        break;

      case 1: // parity
        switch (token[0]) {
          case 'n': serConfig.parity = 0;                  break;
          case 'e': serConfig.parity = PARENB;             break; 
          case 'o': serConfig.parity = (PARENB | PARODD);  break;
          default:
            die("Unrecognised serial parity");
            break;
        }
        break;

      case 2: // bitsize
        temp = atoi(token);
        switch (temp) {
          case 5: serConfig.bitSize = CS5; break;
          case 6: serConfig.bitSize = CS6; break;
          case 7: serConfig.bitSize = CS7; break;
          case 8: serConfig.bitSize = CS8; break;
          default:
            die("Unrecognised serial bit size");
            break;
        }
        break;

      case 3: // stopbits
        temp = atoi(token);
        switch (temp) {
          case 1: serConfig.stopBits = 0;      break;
          case 2: serConfig.stopBits = CSTOPB; break;
          default:
            die("Unrecognised serial stop bits");
            break;
        }
    }
    token = strtok(NULL, SER_SEPARATORS); 
    position++;
  }

  if (position != 4)
    die("Please use '9600,n,8,1' format for serial port setup");
}

/** Setup serial port according to serConfig.
 * Serial port will be setup as non-blocking.
 * Will either fully succeed or die.
 *
 * @param fd Opened fd of the serial port device
 */
void serial_setup(int fd) {
  struct termios tty;

  bzero(&tty, sizeof(tty));
  if (tcgetattr(fd, &tty) != 0)
    die("Can't get serial port current serConfig");

  cfsetispeed(&tty, serConfig.speed);
  cfsetospeed(&tty, serConfig.speed);

  tty.c_iflag &= ~(IXON | IXOFF | IXANY);
  tty.c_oflag = 0;
  tty.c_lflag = 0;

  tty.c_cflag = (tty.c_cflag & ~CSIZE) | serConfig.bitSize;
  tty.c_cflag &= ~(CRTSCTS | CSTOPB | PARENB | PARODD);
  tty.c_cflag |= (serConfig.parity | serConfig.stopBits);
  tty.c_cflag |= CLOCAL | CREAD;

  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = SER_TIMEOUT;

  if (tcsetattr(fd, TCSANOW, &tty) != 0)
    die("Can't setup serial port");
}
