/** @file
 * CLI library configuration
 *
 * @author Piotr S. Staszewski
 */

#ifndef IRPD_CLI
#define IRPD_CLI 1

// Configurable defines

#define CLI_PANELBUF  24    //!< This is used for I/O with panel
#define CLI_CLIENTBUF 1024  //!< Maximum length for input line  
#define CLI_LCDLINES  4     //!< LCD height in lines
#define CLI_LCDCHARS  20    //!< LCD width in chars/bytes

// Public routines

void cli_setup(void);
void cli_loop(void);

#endif
