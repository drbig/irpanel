/** @file
 * CLI processing library
 *
 * This library handles the translation of ASCII lines into
 * commands for the irpanel firmware. It ensures data sanity
 * and provides simple state tracking.
 * As of now it also does IR packet squashing.
 *
 * @author Piotr S. Staszewski
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <unistd.h>

#include "common.h"
#include "cli.h"
#include "irpaneld.h"

// Internal defines

#define PANEL     0                         //!< Panel index into pfds (for readability)
#define CLIENT    1                         //!< Client index into pfds
#define LCD_SIZE  CLI_LCDLINES*CLI_LCDCHARS //!< LCD size in chars/bytes

// Internal variables

static struct pollfd pfds[2]; //!< For polling

static struct state {
  unsigned char x;
  unsigned char y;
  unsigned char dim;
  char buf[LCD_SIZE];
} lcdState;                   //!< Keeps current state of the LCD

static struct packet {
  int addr;
  int cmd;
  int count;
} irpkt;                      //!< For IR packet squashing

static FILE *fPanel;          //!< For formatted output to panel (write-only)
static FILE *fClient;         //!< For formatted output to client (write-only)

static unsigned char bufPanelIn[CLI_PANELBUF];  //!< Panel input buffer
static unsigned char bufPanelOut[CLI_PANELBUF]; //!< Panel output buffer
static char bufClient[CLI_CLIENTBUF];           //!< Client input buffer

// Internal routines

// client reply helpers

/** Send error reply to client
 *
 * @param msg The error message (argument)
 * @private
 */
static void say_error(const char *msg) {
  fprintf(fClient, "error:%s\n", msg);
}

/** Send ok reply to client
 *
 * @private
 */
static void say_ok() {
  fprintf(fClient, "ok\n");
}

// packet IO for panel

/** Read packet from panel
 * This will warn when things are wrong, and if it does everything will
 * probably break. This is not handled as of now.
 * Reads packet into bufPanelIn.
 *
 * @see bufPanelIn
 * @return True if full packet read
 * @private
 */
static bool read_packet() {
  bool ret;
  unsigned char len, pos, tries;

  ret = false;
  len = pos = tries = 0;

  bzero(&bufPanelIn, sizeof(bufPanelIn));

  if (fcntl(fdPanel, F_SETFL, fcntl(fdPanel, F_GETFL) & ~O_NONBLOCK) == -1)
    warn("Can't change PANEL FD to blocking");

  while (read(fdPanel, &len, 1) != 1) {
    warn("Can't read packet length");
    if (++tries > 3) {
      warn("Not trying any more...");
      return ret;
    }
  }

  if ((len < 1) || (len > CLI_PANELBUF))
    warn("Payload length larger than available buffer");
  else {
    while (pos < len)
      if (read(fdPanel, &bufPanelIn[pos], 1) == 1) pos++;
    ret = true;
  }

  if (fcntl(fdPanel, F_SETFL, fcntl(fdPanel, F_GETFL) | O_NONBLOCK) == -1)
    warn("Can't change PANEL FD to non-blocking");

  return ret;
}

/** Send packet to panel
 * Sends contents of bufPanelOut, which has to be properly setup before.
 * It will also check for a reply packet.
 *
 * @see bufPanelOut
 * @see read_packet
 * @return True if everything ok
 * @private
 */
static bool send_packet() {
  if (fwrite(&bufPanelOut, 1, bufPanelOut[0]+1, fPanel) == (bufPanelOut[0]+1)) {
    fflush(fPanel);
    if (!(read_packet() && (bufPanelIn[0] == 'd'))) {
      say_error("firmware error");
      return false;
    }
    return true;
  } else {
    say_error("write failed");
    return false;
  }
}

// input processing

/** Process panel input
 * Do actions based on received packets.
 * Will squash IR packets.
 *
 * @see bufPanelIn
 * @private
 */
static void cli_panel_input() {
  switch (bufPanelIn[0]) {
    case 'i': // IR input
      if (squash > 1) {
        if (irpkt.count > 0) {
          if ((irpkt.addr == bufPanelIn[1]) && (irpkt.cmd == bufPanelIn[2])) {
            if (++irpkt.count >= squash) {
              fprintf(fClient, "ir:%d:%d\n", irpkt.addr, irpkt.cmd);
              irpkt.count = 0;
            }
          } else
            irpkt.count = 1;
        } else irpkt.count++;
        irpkt.addr = bufPanelIn[1];
        irpkt.cmd = bufPanelIn[2];
      } else
        fprintf(fClient, "ir:%u:%u\n", bufPanelIn[1], bufPanelIn[2]);

      fflush(fClient);
      break;

    default:
      note("<< Unknown packet: '%s'", bufPanelIn);
      break;
  }
}

/** Process client input
 * Works on chunks of the input string.
 *
 * @param line The line to process (null-terminated)
 * @private
 */
static void cli_client_input(char *line) {
  int a, b, c;

  dbg(printf("CLI << %s\n", line));
  bzero(&bufPanelOut, CLI_PANELBUF);

  switch (line[0]) {
    case 'q': // query LCD state
      dbg(printf(">> CMD: QUERY\n"));
      a = strlen(line) - 2;
      if (a != 1)
        say_error("argument length error");
      else
        switch (line[2]) {
          case 'p': // position
            fprintf(fClient, "ok:%u:%u\n", lcdState.x, lcdState.y);
            break;

          case 'd': // dim value
            fprintf(fClient, "ok:%u\n", lcdState.dim);
            break;

          case 'l': // contents of the LCD (single line)
            fprintf(fClient, "ok:%s\n", lcdState.buf);
            break;

          default:
            note(">> Unknown query");
            fprintf(fClient, "fail:command unknown\n");
            break;
        }
      break;

    case 'p': // print line to LCD
      dbg(printf(">> CMD: PRINT LINE\n"));
      a = strlen(line) - 2;
      if ((a < 1) || (a > LCD_SIZE))
        say_error("argument length error");
      else {
        for (b = 0; b < a;) {
          c = a - b;
          if (c > (CLI_LCDCHARS - lcdState.x))
            c = CLI_LCDCHARS - lcdState.x;

          bufPanelOut[0] = c+1;
          bufPanelOut[1] = 'p';
          strncpy((char *)&bufPanelOut+2, line+b+2, c);

          if (!send_packet()) return;
          else {
            strncpy((char *)&lcdState.buf+(lcdState.x + CLI_LCDCHARS*lcdState.y), line+b+2, c);
            b += c;

            // handle moving to the right next line
            if ((lcdState.x + c) >= CLI_LCDCHARS) {
              bufPanelOut[0] = 3;
              bufPanelOut[1] = 'g';
              bufPanelOut[2] = 0;
              bufPanelOut[3] = lcdState.y + 1;

              if (bufPanelOut[3] >= CLI_LCDLINES)
                bufPanelOut[3] = 0;

              if (!send_packet()) return;
              else {
                lcdState.x = bufPanelOut[2];
                lcdState.y = bufPanelOut[3];
              }
            } else
              lcdState.x += c;
          }
        }
        say_ok();
      }
      break;

    case 'g': // move cursor to specified position
      if (sscanf(line, "g:%d:%d", &a, &b) != 2)
        say_error("parse failed");
      else {
        dbg(printf(">> CMD: GOTO X=%d Y=%d\n", a, b));
        if (((a < 0) || (a > (CLI_LCDCHARS-1))) || ((b < 0) || (b > (CLI_LCDLINES-1))))
          say_error("argument out of range");
        else {
          bufPanelOut[0] = 3;
          bufPanelOut[1] = 'g';
          bufPanelOut[2] = a;
          bufPanelOut[3] = b;
          if (send_packet()) {
            lcdState.x = a;
            lcdState.y = b;
            say_ok();
          }
        }
      }
      break;

    case 'd': // set dim value
      if (sscanf(line, "d:%d", &a) != 1)
        say_error("parse failed");
      else {
        dbg(printf(">> CMD: BRIGHTNESS=%d\n", a));
        if ((a < 0) || (a > 255)) {
          say_error("argument out of range");
          break;
        }
        bufPanelOut[0] = 2;
        bufPanelOut[1] = 'd';
        bufPanelOut[2] = a;
        if (send_packet()) {
          lcdState.dim = a;
          say_ok();
        }
      }
      break;

    case 'c': // clear LCD
      dbg(printf(">> CMD: CLEAR\n"));
      bufPanelOut[0] = 1;
      bufPanelOut[1] = 'c';
      if (send_packet()) {
        lcdState.x = lcdState.y = 0;
        memset(&lcdState.buf, ' ', LCD_SIZE);
        say_ok();
      }
      break;

    case 'h': // home LCD
      dbg(printf(">> CMD: HOME\n"));
      bufPanelOut[0] = 1;
      bufPanelOut[1] = 'h';
      if (send_packet()) {
        lcdState.x = lcdState.y = 0;
        say_ok();
      }
      break;

    default:
      note(">> Unknown command");
      fprintf(fClient, "fail:command unknown\n");
      break;
  }
}

// Public routines

/** Setup the CLI library
 */
void cli_setup() {
  pfds[PANEL].fd = fdPanel;
  pfds[PANEL].events = POLLIN;
  pfds[CLIENT].events = POLLIN;

  if ((fPanel = fdopen(fdPanel, "w")) == NULL)
    die("Error converting PANEL FD to FILE");

  bzero(&lcdState, sizeof(lcdState));
  memset(&lcdState.buf, ' ', LCD_SIZE);
  lcdState.dim = 128;
}

/** Main processing loop
 * Requires appropriate FDs setup.
 *
 * @see fdPanel
 * @see fdClient
 */
void cli_loop() {
  char *line;
  int count;

  pfds[CLIENT].fd = fdClient;
  irpkt.count = 0;

  if ((fClient = fdopen(fdClient, "w")) == NULL) {
    warn("Error converting CLIENT FD to FILE");
    return;
  }

  while ((count = read(fdPanel, &bufClient, sizeof(bufClient))) > 0) {
    note("Dumping stale panel data...");
  }

  while (true) {
    if (poll(pfds, 2, -1) < 0) {
      warn("Error on poll");
      break;
    }

    if (pfds[PANEL].revents & POLLHUP)
      die("PANEL EOF");

    if (pfds[CLIENT].revents & POLLHUP) {
      note("Client EOF (1)");
      break;
    }

    if (pfds[PANEL].revents & POLLIN)
      if (read_packet()) cli_panel_input();

    if (pfds[CLIENT].revents & POLLIN) {
      bzero(&bufClient, sizeof(bufClient));

      if ((count = read(fdClient, &bufClient, sizeof(bufClient))) == 0) {
        note("Client EOF (2)");
        break;
      } else {
        dbg(printf("CLIENT << %s\n", bufClient));
        if (bufClient[count-1] == '\n') {
          line = strtok((char *)&bufClient, "\n");
          while (line != NULL) {
            cli_client_input(line);
            line = strtok(NULL, "\n");
          }
        } else
          fprintf(fClient, "fail:command error\n");
      }
      fflush(fClient);
    }
  }
}
