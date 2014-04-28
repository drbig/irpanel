/** @file
 * Common code and macros
 *
 * @author Piotr S. Staszewski
 */

#ifndef IRPD_COMMON
#define IRPD_COMMON 1

// Internal defines

// helpers
#define GLUE_NX(a, b) a##b          //!< Glue into symbol, no expansion
#define GLUE(a, b)    GLUE_NX(a, b) //!< Glue into symbol, with expansion
#define STR_NX(x)     #x            //!< Make string, no expansion
#define STR(x)        STR_NX(x)     //!< Make string, with expansion

// debugging macros
#ifdef DEBUG
  #define dbg(code) code;
#else
  #define dbg(code)
#endif

// Public routines

void die(const char *msg);
void warn(const char *msg);

#endif
