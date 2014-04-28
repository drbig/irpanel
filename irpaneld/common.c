/** @file
 * Common code
 *
 * This may actualy be useful outside this project.
 *
 * @author Piotr S. Staszewski
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <time.h>

#include "common.h"

// Public variables

bool cmnStamp = false;

// Private variables

static char timestamp[128];

// Private routines

void stamp(FILE *out) {
  struct tm *tms;
  time_t t;

  if (cmnStamp) {
    t = time(NULL);
    if (((tms = localtime(&t)) != NULL) &&
        (strftime(timestamp, sizeof(timestamp), CMN_TIMESTAMP, tms) > 1))
      fprintf(out, "%s ", timestamp);
  }
}

// Public routines

void note(const char *fmt, ...) {
  va_list args;
  char buffer[1024];

  va_start(args, fmt);
  if (vsnprintf((char *)&buffer, sizeof(buffer), fmt, args) >= sizeof(buffer)) {
    strcpy((char *)&buffer+(sizeof(buffer)-5), "...");
    buffer[sizeof(buffer)-1] = 0;
  }
  va_end(args);

  stamp(stdout);
  printf("INFO\t%s\n", buffer);
  fflush(stdout);
}

/** Print error message and die
 * Will check errno.
 *
 * @param msg The error message
 */
void die(const char *msg) {
  stamp(stderr);
  fprintf(stderr, "FATAL\t");
  if (errno) {
    perror(msg);
    exit(2);
  } else {
    fprintf(stderr, "ABORT: %s\n", msg);
    exit(1);
  }
}

/** Print warning message
 * Will check errno.
 *
 * @param msg The warning message
 */
void warn(const char *msg) {
  stamp(stderr);
  fprintf(stderr, "WARN\t");
  if (errno)
    perror(msg);
  else
    fprintf(stderr, "WARN: %s\n", msg);
  fflush(stderr);
}
