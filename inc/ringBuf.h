#ifndef __ringBuf_h__
#define __ringBuf_h__


#define RINGBUFFER_SIZE       5000

#define BUF_EMPTY               0
#define BUF_FULL                1

//TODO:  change struct ringbuffer_s to void * in function parameters
//       remove structure definition from headerfile

#pragma pack(1)
typedef struct ringbuffer_s {
    unsigned char buffer[RINGBUFFER_SIZE];
    unsigned int size;
    unsigned int fill;
    unsigned char *read;
    unsigned char *write;
    pthread_mutex_t mutex;
}ringbuffer_t;
#pragma pack()



struct ringbuffer_s *ringbuffer_init(void);
int ringbuffer_empty(struct ringbuffer_s *rb);
int ringbuffer_full(struct ringbuffer_s *rb);
int ringbuffer_read(struct ringbuffer_s *rb, unsigned char* buf, unsigned int len);
int ringbuffer_write(struct ringbuffer_s *rb, unsigned char* buf, unsigned int len);
int ringbuffer_currentSize(struct ringbuffer_s *rb);
int ringbuffer_clear(struct ringbuffer_s *rb);


#endif
