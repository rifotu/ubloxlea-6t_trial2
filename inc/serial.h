#ifndef __serialPort_h__
#define __serialPort_h__



#pragma pack(1)
typedef struct serialPort_s
{
    int fd;
    int baudRate;
    int parity;
    uint8_t ticksBtwReceive; 

}serialPort_t;

#pragma pack()

struct serialPort_s *serialPort_init(char *path, int baudrate, int parity);
int serialPort_deinit(struct serialPort_s *sp);
int serialPort_isRxSilent(struct serialPort_s *sp);



#endif
