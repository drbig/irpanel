/** @file
 * IRPanel daemon configurable defines
 *
 * @author Piotr S. Staszewski
 */

#ifndef IRPD_CONFIG
#define IRPD_CONFIG 1

// Configurable defines

#define DEF_DEV     "/dev/ttyUSB0"  //!< Default device to open
#define DEF_MODE    "9600,n,8,1"    //!< Default serial port mode
#define DEF_SQUASH  2               //!< Default squash for IR packets
#define DEF_PID     "irpaneld.pid"  //!< Default pid file
#define DEF_LOG     "irpaneld.log"  //!< Default log file

// Public variables

extern int fdPanel;   //!< FD for the panel connection
extern int fdClient;  //!< FD for the client communication
extern int squash;    //!< For IR packet squashing

#endif
