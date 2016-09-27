/*
 *   Definitions for configuration related parameters
 */


#ifndef _CONFIG_H
#define _CONFIG_H

#include <termios.h>
#include <unistd.h>

/*   Socket Related Settings              */
#define SOCK_PATH "/tmp/bbbrf.sock"


/*   Log Messages Related Settings        */
#define DBG_LOG_MSG_PATH "/tmp/aidGps.log"

/*   Serial Port Related Settings         */
#define SERIAL_PORT       "/dev/ttyS8"
#define SERIAL_BAUD_RATE  B9600
#define NO_PARITY         0
#define UART_RX_BUF_SIZE  150

/*              Misc                        */

/*  incoming eph+alm+ini+hui+posllh ~= 4800 */
#define SCRATCHPAD_BUF_SIZE  5000

#endif
