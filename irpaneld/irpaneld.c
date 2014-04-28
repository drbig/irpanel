/** @file
 * IRPanel daemon code
 *
 * Provides a simple layer between the outside world and the firmware.
 *
 * @note
 * Left debbugging statements in.
 *
 * @todo Test with valgrind
 * @todo Smarter error handling, particularly EOFs and read_packet
 *
 * @author Piotr S. Staszewski
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "common.h"
#include "cli.h"
#include "irpaneld.h"
#include "serial.h"

// Internal defines

#define MODE_TCP  1 //!< Use TCP sockets
#define MODE_UNIX 2 //!< Use UNIX sockets

// fix the discrepancy between documentation and actual code
#ifndef UNIX_PATH_MAX
  #define UNIX_PATH_MAX sizeof(sun.sun_path)
#endif

// Private variables

static int mode;
static int fdServer;
static volatile bool run;

// Public variables

int fdPanel;
int fdClient;
int squash;

// Private routines

void handler_sig(int signum) {
  fprintf(stderr, "\nCaught signal %d, quitting...\n", signum);

  close(fdClient);
  close(fdServer);
  close(fdPanel);

  run = false;
}

/** Print usage.
 * @param name Name of the command
 */
void usage(char *name) {
  fprintf(stderr, "  Usage: %s [options...] (-t HOST:PORT|-u PATH)\n", name);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "\t-b         - fork into background (default: false)\n");
  fprintf(stderr, "\t-p LOG     - where to write PID (default; "STR(DEF_PID)")\n");
  fprintf(stderr, "\t-l PID     - where to write LOG (default: "STR(DEF_LOG)")\n");
  fprintf(stderr, "\t-d DEVICE  - path to serial port device (default: "STR(DEF_DEV)")\n");
  fprintf(stderr, "\t-m MODE    - serial port mode (default: "STR(DEF_MODE)")\n");
  fprintf(stderr, "\t-s NUM     - squash NUM IR packets (default: "STR(DEF_SQUASH)")\n");
  fprintf(stderr, "\nAnd one of the following:\n");
  fprintf(stderr, "\t-t HOST:PORT - listen on a TCP socket on HOST:PORT\n");
  fprintf(stderr, "\t-u PATH      - listen on a UNIX domain socket at PATH\n");
  exit(1);
}

// Main routine

int main(int argc, char *argv[]) {
  struct sockaddr_in sin;
  struct sockaddr_un sun;
  struct hostent *he;
  socklen_t slen;
  pid_t pid, sid;
  FILE *fLog, *fPid;
  bool background;
  char *modeArg, *device, *serialMode, *pidPath, *logPath;
  int opt, num;

  signal(SIGINT, handler_sig);
  signal(SIGTERM, handler_sig);

  if (argc < 3) usage(argv[0]);

  cmnStamp = false;

  squash = DEF_SQUASH;
  background = false;
  run = true;
  modeArg = device = serialMode = pidPath = logPath = NULL;
  he = NULL;
  mode = fdServer = fdClient = 0;

  while ((opt = getopt(argc, argv, "bp:l:d:m:s:t:u:")) != -1)
    switch (opt) {
      case 'b': background = true;                  break;
      case 'p': pidPath = optarg;                   break;
      case 'l': logPath = optarg;                   break;
      case 'd': device = optarg;                    break;
      case 'm': serialMode = optarg;                break;
      case 's': squash = atoi(optarg);              break;
      case 't': mode = MODE_TCP; modeArg = optarg;  break;
      case 'u': mode = MODE_UNIX; modeArg = optarg; break;
      default:  usage(argv[0]);                     break;
    }

  if (mode == 0) usage(argv[0]);

  if (device == NULL) {
    device = malloc(sizeof(DEF_DEV));
    strcpy(device, DEF_DEV);
  }
  dbg(printf("DEVICE: %s\n", device));

  if (serialMode == NULL) {
    serialMode = malloc(sizeof(DEF_MODE));
    strcpy(serialMode, DEF_MODE);
  }
  dbg(printf("SERIAL MODE: %s\n", serialMode));
  serial_parse(serialMode);

  switch (mode) {
    case MODE_UNIX:
      bzero(&sun, sizeof(sun));
      sun.sun_family = AF_UNIX;

      num = strlen(modeArg);
      if ((num < 1) || (num > UNIX_PATH_MAX))
        die("Unix socket path to short/long");
      if (access(modeArg, F_OK) == 0)
        warn("File exists at the specified path");
      strncpy((char *)&sun.sun_path, modeArg, UNIX_PATH_MAX);
      dbg(printf("socket at: %s\n", sun.sun_path));
      break;

    case MODE_TCP:
      bzero(&sin, sizeof(sin));
      sin.sin_family = AF_INET;

      if ((he = gethostbyname(strtok(modeArg, ":"))) == NULL)
        die("Can't find IP for the supplied HOST");
      memcpy(he->h_addr_list[0], &sin.sin_addr, sizeof(sin.sin_addr));

      num = atoi(strtok(NULL, ":"));
      if ((num < 1) || (num > 65535))
        die("Invalid PORT number");
      sin.sin_port = htons(num);
      dbg(printf("host: '%s' port: '%d'\n", he->h_name, num));
      break;
    default:
      usage(argv[0]);
      break;
  }

  if ((fdPanel = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    die("Can't open serial port device");
  if (flock(fdPanel, LOCK_EX) != 0)
    die("Can't lock serial port");
  serial_setup(fdPanel);

  cli_setup();

  switch (mode) {
    case MODE_UNIX:
      if ((fdServer = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
        die("Can't create a UNIX socket");

      if (bind(fdServer, (struct sockaddr *)&sun, sizeof(sun)) != 0)
        die("Can't bind socket to specified PATH");
      break;

    case MODE_TCP:
      if ((fdServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        die("Can't create a TCP socket");

      if (bind(fdServer, (struct sockaddr *)&sin, sizeof(sin)) != 0)
        die("Can't bind socket to specified HOST:PORT");

      setsockopt(fdServer, SOL_SOCKET, SO_REUSEADDR, &num, sizeof(num));
      break;
  }

  if (listen(fdServer, 1) != 0)
    die("Can't listen on socket");

  if (background) {
    if (pidPath == NULL) {
      pidPath = malloc(sizeof(DEF_PID));
      strcpy(pidPath, DEF_PID);
    }
    dbg(printf("pid path: %s\n", pidPath));

    if (access(pidPath, F_OK) == 0)
      die("PID file exists, already running?");
    if ((fPid = fopen(pidPath, "w")) == NULL)
      die("Can't open PID file");

    if (logPath == NULL) {
      logPath = malloc(sizeof(DEF_LOG));
      strcpy(logPath, DEF_LOG);
    }
    dbg(printf("log path: %s\n", logPath));

    if ((fLog = fopen(logPath, "a")) == NULL)
      die("Can't open log file");
    if (dup2(fileno(fLog), STDOUT_FILENO) == -1)
      die("Can't redirect STDOUT");
    if (dup2(fileno(fLog), STDERR_FILENO) == -1)
      die("Can't redirect STDRR");
    fclose(fLog);

    cmnStamp = true;

    pid = fork();
    if (pid < 0)
      die("Can't fork into background");
    if (pid > 0)
      exit(0);

    if ((sid = setsid()) < 0)
      die("Can't request new sid");

    note("Daemon running with PID: %d", sid);
    fprintf(fPid, "%d\n", sid);
    fclose(fPid);

    note("Panel device: %s", device);
    note("Serial config: %s", serialMode);
    switch (mode) {
      case MODE_UNIX:
        note("UNIX socket at: %s", sun.sun_path);               break;
      case MODE_TCP:
        note("TCP socket at: %s:%d", he->h_name, ntohs(sin.sin_port)); break;
    }
  }

  while (run) {
    dbg(printf("Waiting for client...\n"));
    switch (mode) {
      case MODE_UNIX:
        if (((fdClient = accept(fdServer, (struct sockaddr *)&sun, &slen)) < 0) && run)
          die("Error on accept");
        break;

      case MODE_TCP:
        if ((fdClient = accept(fdServer, (struct sockaddr *)&sin, &slen)) < 0) {
          if (run) die("Error on accept");
          else break;
        }
        note("Client from: %s:%d", inet_ntoa(sin.sin_addr), ntohs(sin.sin_port));
        break;
    }

    if (!run) break;

    note("Client connected");
    cli_loop();
    note("Client disconnected");
    close(fdClient);
  }

  if (mode == MODE_UNIX)
    if (unlink(sun.sun_path) != 0)
      warn("Can't remove UNIX socket");

  if (background)
    if (unlink(pidPath) != 0)
      warn("Can't remove PID file");

  exit(0);
}
