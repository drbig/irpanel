/** @file
 * Common code
 *
 * This may actualy be useful outside this project.
 *
 * @author Piotr S. Staszewski
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

// Public routines

/** Print error message and die
 * Will check errno.
 *
 * @param msg The error message
 */
void die(const char *msg) {
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
  if (errno)
    perror(msg);
  else
    fprintf(stderr, "WARN: %s\n", msg);
}
