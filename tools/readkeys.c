#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "common.h"

#ifndef UNIX_PATH_MAX
  #define UNIX_PATH_MAX sizeof(sun.sun_path)
#endif

struct packet {
  int addr;
  int cmd;
} pkt;

char buffer[256];
int fdServer;

void handler_sigint(int signum) {
  fprintf(stderr, "\nCaught Ctrl+C, quitting...\n");
  exit(1);
}

bool read_packet() {
  int num;

  while ((num = recv(fdServer, &buffer, sizeof(buffer), 0)) > 0) {
    buffer[num] = 0;
    //dbg(fprintf(stderr, "%s", buffer));
    if (sscanf((char *)&buffer, "ir:%d:%d", &pkt.addr, &pkt.cmd) == 2)
      return true;
    else
      return false;
  }

  return false;
}

void usage(char *name) {
  fprintf(stderr, "  Usage: %s [-r] [-s NUM] (-t HOST:PORT|-u PATH)\n", name);
  fprintf(stderr, "\nOptions:\n");
  fprintf(stderr, "\t-r           - raw mode, just print packets\n");
  fprintf(stderr, "\nAnd one of the following:\n");
  fprintf(stderr, "\t-t HOST:PORT - listen on a TCP socket on HOST:PORT\n");
  fprintf(stderr, "\t-u PATH      - listen on a UNIX domain socket at PATH\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  struct sockaddr_in sin;
  struct sockaddr_un sun;
  struct sockaddr *addr;
  struct hostent *he;
  socklen_t slen;
  size_t len;
  char *line;
  bool mode, raw;
  int opt, num;

  signal(SIGINT, handler_sigint);

  if (argc < 3) usage(argv[0]);

  addr = NULL;
  line = NULL;

  fdServer = slen = len = 0;
  mode = raw = false;

  while ((opt = getopt(argc, argv, "rt:u:")) != -1)
    switch (opt) {
      case 'r':
        raw = true;
        break;

      case 't':
        if (mode) usage(argv[0]);

        bzero(&sin, sizeof(sin));
        sin.sin_family = AF_INET;

        if ((he = gethostbyname(strtok(optarg, ":"))) == NULL)
          die("Can't find IP for the supplied HOST");
        memcpy(he->h_addr_list[0], &sin.sin_addr, sizeof(sin.sin_addr));

        num = atoi(strtok(NULL, ":"));
        if ((num < 1) || (num > 65535))
          die("Invalid PORT number");
        sin.sin_port = htons(num);

        if ((fdServer = socket(AF_INET, SOCK_STREAM, 0)) < 0)
          die("Can't create a TCP socket");

        addr = (struct sockaddr *)&sin;
        slen = sizeof(sin);
        mode = true;
        dbg(fprintf(stderr, "SOCKET: %s:%d\n", he->h_name, num));
        break;

      case 'u':
        if (mode) usage(argv[0]);

        bzero(&sun, sizeof(sun));
        sun.sun_family = AF_UNIX;

        num = strlen(optarg);
        if ((num < 1) || (num > UNIX_PATH_MAX))
          die("Unix socket path to short/long");
        strncpy((char *)&sun.sun_path, optarg, UNIX_PATH_MAX);

        if ((fdServer = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
          die("Can't create a UNIX socket");

        addr = (struct sockaddr *)&sun;
        slen = sizeof(sun);
        mode = true;
        dbg(fprintf(stderr, "SOCKET: %s\n", sun.sun_path));
        break;

      default:
        usage(argv[0]);
        break;
    }

  if (!mode) usage(argv[0]);

  if (connect(fdServer, addr, slen) != 0)
    die("Can't connect");

  fprintf(stderr, "  MODE: %s\n", raw ? "RAW" : "NORMAL");

  if (raw) {
    while (true)
      if (read_packet())
        printf("%d,%d\n", pkt.addr, pkt.cmd);
  } else {
    while ((num = getline(&line, &len, stdin)) != -1) {
      line[num-1] = 0;

      num = 0;
      do {
        fprintf(stderr, "%s = ", line);
        if (read_packet()) {
          fprintf(stderr, "%d:%d\n", pkt.addr, pkt.cmd);
          printf("\"%s\",%d,%d\n", line, pkt.addr, pkt.cmd);
          num = 1;
        } else {
          fprintf(stderr, "Try again...\n");
        }
      } while (num != 1);

    }
    free(line);
  }

  close(fdServer);
  exit(0);
}
