#include <unistd.h>
#include <stdio.h>      // standard input / output functions
#include <stdlib.h>
#include <string.h>     // string function definitions
#include <fcntl.h>      // File control definitions
#include <errno.h>      // Error number definitions
#include <termios.h>    // POSIX terminal control definitions
#include <time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/un.h>

#include "serial.h"


// Structure definitions

// Public function prototypes

// Private function prototypes
static int set_interface_attribs (int fd, int speed, int parity);




struct serialPort_s *serialPort_init(char *path, int baudRate, int parity)
{
    struct serialPort_s *sp = NULL;
    int ret;

    sp = (struct serialPort_s *)malloc(sizeof(struct serialPort_s));
    if(NULL == sp){
        return NULL;
    }

    sp->fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY);
    if(sp->fd < 0)
    {
        printf("can't open serial port\n");
        exit(0);
    }
    sp->baudRate = baudRate;
    sp->parity   = parity;
    sp->ticksBtwReceive = 0;

    ret = set_interface_attribs(sp->fd, sp->baudRate, sp->parity);
    if(-1 == ret){
        close(sp->fd);
        return NULL;
    }

    return sp;
}

int serialPort_isRxSilent(struct serialPort_s *sp)
{
    uint8_t tick1, tick2;
    uint8_t overlapLimit = 255;

    while(1){

        tick1 = sp->ticksBtwReceive;
        sleep(4);
        tick2 = sp->ticksBtwReceive;

        if(overlapLimit - tick1 > 10)
        {
            if( tick2 - tick1 > 5 ){
                return 1;
            }else{
                return -1;
            }
        }else{
            sleep(2);
        }
    } // while(1)
}

int serialPort_deinit(struct serialPort_s *sp)
{
    close(sp->fd);
    free(sp);

    return 0;
}



static int set_interface_attribs (int fd, int speed, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0)
    {
        printf ("error %d from tcgetattr", errno);
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    // Enable the receiver and set local mode...
    tty.c_cflag |= (CLOCAL | CREAD);

    // No parity (8N1)
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    // Disable hardware flow control:
    tty.c_cflag &= ~CRTSCTS;

    // Disable software flow control:
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);

    // Configure line as raw input:
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    // Configure line as raw output:
    tty.c_oflag &= ~OPOST;

    // For details about blocking in non canonical (binary mode)
    // check out http://unixwiz.net/techtips/termios-vmin-vtime.html
    tty.c_cc[VMIN]  = 0;            // no need to wait for any bytes in the buffer
    tty.c_cc[VTIME] = 1;            // 0.5 seconds read timeout


    if (tcsetattr (fd, TCSANOW, &tty) != 0)
    {
        printf ("error %d from tcsetattr", errno);
        return -1;
    }
    return 0;
}


