/* uart messaging related functions */

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>

#include "ringBuf.h"

/*   GLOBAL VARIABLES           */
// For debugging purposes, a default uart handle
// will be assigned to uart1



/*************    Ringbuffer related functions  ************/ 
int ringbuffer_empty(struct ringbuffer_s *rb)
{
	/* It's empty when the read and write pointers are the same. */
	if (0 == rb->fill) {
		return 1;
	}else {
		return 0;
	}
}


int ringbuffer_full(struct ringbuffer_s *rb)
{
	/* It's full when the write ponter is 1 element before the read pointer*/
	if (rb->size == rb->fill) {
		return 1;
	}else {
		return 0;
	}
}

int ringbuffer_currentSize(struct ringbuffer_s *rb)
{
	return rb->fill;
}



int ringbuffer_read(struct ringbuffer_s *rb, unsigned char* buf, unsigned int len)
{
	
    pthread_mutex_lock( &(rb->mutex));
	if (rb->fill >= len) {
		// in one direction, there is enough data for retrieving
		if (rb->write > rb->read) {
			memcpy(buf, rb->read, len);
			rb->read += len;
		}else if (rb->write < rb->read) {
			int len1 = rb->buffer + rb->size - 1 - rb->read + 1;
			if (len1 >= len) {
				memcpy(buf, rb->read, len);
				rb->read += len;
			} else {
				int len2 = len - len1;
				memcpy(buf, rb->read, len1);
				memcpy(buf + len1, rb->buffer, len2);
				rb->read = rb->buffer + len2; // Wrap around
			}
		}
		rb->fill -= len;
        pthread_mutex_unlock( &(rb->mutex));
		return len;
	} else	{
        pthread_mutex_unlock( &(rb->mutex));
		return 0;
	}
}

int ringbuffer_clear(struct ringbuffer_s *rb)
{
    pthread_mutex_lock( &(rb->mutex));
    
	rb->size   = RINGBUFFER_SIZE;
	memset(rb->buffer, 0, rb->size);
	rb->fill   = 0;
	rb->read   = rb->buffer;
	rb->write  = rb->buffer;
    pthread_mutex_unlock( &(rb->mutex));

    return 0;
}

int ringbuffer_write(struct ringbuffer_s *rb, unsigned char* buf, unsigned int len)
{
	
    pthread_mutex_lock( &(rb->mutex));
	if (rb->size - rb->fill < len) {
        pthread_mutex_unlock( &(rb->mutex));
		return 0;
	}
	else {
		if (rb->write >= rb->read) {
			int len1 = rb->buffer + rb->size - rb->write;
			if (len1 >= len) {
				memcpy(rb->write, buf, len);
				rb->write += len;
			} else {
				int len2 = len - len1;
				memcpy(rb->write, buf, len1);
				memcpy(rb->buffer, buf+len1, len2);
				rb->write = rb->buffer + len2; // Wrap around
			}
		} else {
			memcpy(rb->write, buf, len);
			rb->write += len;
		}
		rb->fill += len;
        pthread_mutex_unlock( &(rb->mutex));
		return len;
	}
}



struct ringbuffer_s * ringbuffer_init(void)
{
	struct ringbuffer_s *rb = NULL;

    rb = (struct ringbuffer_s *)malloc(sizeof(struct ringbuffer_s));
    if(NULL == rb){
        printf("error\n");
    }

	rb->size   = RINGBUFFER_SIZE;
	memset(rb->buffer, 0, rb->size);
	rb->fill   = 0;
	rb->read   = rb->buffer;
	rb->write  = rb->buffer;
    pthread_mutex_init(&(rb->mutex),NULL);

    return rb;

}
