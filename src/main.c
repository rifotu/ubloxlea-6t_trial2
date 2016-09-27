
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "gps.h"
#include "ubx.h"
#include "ubx-parse.h"
#include "config.h"
#include "serial.h"
#include "debug.h"
#include "ringBuf.h"
#include "list.h"


/* Global Definitions   */

FILE *fpDbg_G; 
int  dbgLevel_G;

/* Structure Definitions */
#pragma pack(1)
typedef struct monitor_s
{
    struct llist* llistTxCommands;
    struct ringbuffer_s * rbUartIn_p;
    struct ringbuffer_s * rbUbxMsg_p;
    struct serialPort_s * serialPort_p;
    struct msgStrmCheck_s msgChk; 

}monitor_t;

#pragma pack()


static struct monitor_s * prep_monitoringStruct(void);
static int setDbgLogs(void);
static void freeNode(void *data);
static void getMissingMessages(struct monitor_s * mon_p, uint8_t *scratchpad);
static int silenceNmea(struct monitor_s *mon_p);


//static int do_rrlp(struct gps_assist_data *gps)
//{
//	struct rrlp_assist_req ar;
//	void *pdus[64];
//	int len[64];
//	int i, j, rv;
//
//	char *test = "\x28\x00\x80\x10\x01\x32\x00\x19\x4F\x07\x15\x04"; // //	rrlp_decode_assistance_request(&ar, test, 12); //	printf("%08x %016llx\n", ar.req_elems, (long long unsigned) ar.eph_svs); // //	ar.req_elems = -1; //	ar.eph_svs = -1LL; //	rv = rrlp_gps_assist_pdus(gps, &ar, pdus, len, 64); //	printf("%d\n", rv);
//	for (i=0; i<rv; i++) {
//		printf("%p %d\n", pdus[i], len[i]);
//        for(j=0; j < len[i]; j++){
//            printf("%X", *( ((uint8_t *)pdus[i]) + j ));
//        }
//        printf("\n");
//	}
//	return 0;
//}


static int setDbgLogs(void)
{
    fpDbg_G     = fopen(DBG_LOG_MSG_PATH, "w+");
    dbgLevel_G  = LOG_INFO;
    LOG(LOG_INFO, "Initialized Log Messages");

    return 0;
}
  
static struct monitor_s * prep_monitoringStruct(void)
{
    struct monitor_s *mon = NULL;

    mon = (struct monitor_s *)malloc(sizeof(struct monitor_s));

    mon->llistTxCommands = create_list();
    mon->rbUartIn_p = ringbuffer_init();
    mon->rbUbxMsg_p = ringbuffer_init();
    mon->serialPort_p = serialPort_init(SERIAL_PORT, SERIAL_BAUD_RATE, NO_PARITY);

    // clean all acknowledgments and disable message sending
    memset( &(mon->msgChk), 0, sizeof( struct msgStrmCheck_s));
    
    return mon;
}


static void freeNode(void *data)
{
    free( (uint8_t *)data);
}

static void getMissingMessages(struct monitor_s * mon_p, uint8_t *scratchpad)
{
    //struct ubx_hdr *ubxMsg_p = NULL;
    int rb_size = 0;

    memset(scratchpad, 0, sizeof(uint8_t) * SCRATCHPAD_BUF_SIZE);

    prepAidMissingPollMsgs(mon_p->llistTxCommands, &(mon_p->msgChk));

    while( size(mon_p->llistTxCommands) ){
        LOG(LOG_INFO, "number of commands in txlist: %d",size(mon_p->llistTxCommands));
        sleep(2);
    }
    LOG(LOG_INFO, "missing requests all sent now");

    // we have sent all missing commands, now read from ringbuffer
    rb_size = ringbuffer_currentSize(mon_p->rbUartIn_p);
    ringbuffer_read(mon_p->rbUartIn_p, scratchpad, rb_size);
    LOG(LOG_INFO, "read incoming values from ringbuffer to scratchpad");

    /* Parse Each Message */
    for(int i=0; i < rb_size; )
    {
        int rv;
        rv = parseUartInput_4_UbxMsg(scratchpad + i, rb_size - i);
        if(rv < 0){
            i++;	/* Invalid message: try one byte later */
        }else{
            /* got a valid message, copy it to ring buffer */
            ringbuffer_write(mon_p->rbUbxMsg_p, scratchpad + i, rv);
            updateValidUbxMsgList(scratchpad + i, &(mon_p->msgChk) );
            // increment the pointer 
            i += rv; 
        }
    }

    // clear scratchpad 
    memset(scratchpad, 0, SCRATCHPAD_BUF_SIZE); 
}


static int silenceNmea(struct monitor_s *mon_p)
{
    do{
        // prepare nmea silencer commands
        prepNmeaSilencerMsgs(mon_p->llistTxCommands);
        // wait until all commands are issued
        while( size(mon_p->llistTxCommands) ){ sleep(1); }

    //}while( serialPort_isRxSilent(mon_p->serialPort_p) == -1);
    }while(0);

    LOG(LOG_INFO, "serial port is silent now");
    // clear uartRx ringbuffer
    ringbuffer_clear(mon_p->rbUartIn_p);

    return 0;
}




static void getAidMessages(struct monitor_s * mon_p, uint8_t *scratchpad)
{
    int rb_size = 0;
    // ublox lea-6t is configured, now poll for AID messages
    prepAidPollMsgs(mon_p->llistTxCommands);


    // after the last command is sent, in 2 seconds,
    // all data should be available
    while( size(mon_p->llistTxCommands)){ 
        LOG(LOG_INFO, "%d msgs left in tx list", size(mon_p->llistTxCommands));
        sleep(2);
    }

    // we have sent all aid poll commands, now read from ringbuffer
    rb_size = ringbuffer_currentSize(mon_p->rbUartIn_p);
    ringbuffer_read(mon_p->rbUartIn_p, scratchpad, rb_size); 

    /* Parse each message */
    for(int i=0; i < rb_size; )
    {
        int rv;
        rv = parseUartInput_4_UbxMsg(scratchpad + i, rb_size - i);
        if(rv < 0){
            i++;	/* Invalid message: try one byte later */
        }else{
            /* got a valid message, copy it to ring buffer */
            ringbuffer_write(mon_p->rbUbxMsg_p, scratchpad + i, rv);
            updateValidUbxMsgList(scratchpad + i, &(mon_p->msgChk) );
            // increment the pointer 
            i += rv; 
        }
    }

    //FIXME:  if there are any remaining data left in the buffer, then
    //       they will all be cleared away below!!
    // clear scratchpad 
    memset(scratchpad, 0, SCRATCHPAD_BUF_SIZE); 

}



static void *control_f(void *arg)
{
    struct monitor_s *mon_p = (struct monitor_s *)arg;
	struct gps_assist_data gps;
    uint8_t scratchpad[SCRATCHPAD_BUF_SIZE];

    memset(scratchpad, 0, sizeof(uint8_t) * SCRATCHPAD_BUF_SIZE);
	memset(&gps, 0x00, sizeof(gps));

    silenceNmea(mon_p);


    while(1){
        
        LOG(LOG_INFO, "asking for aid messages");
        getAidMessages(mon_p, scratchpad);

        for(int j = 0; j < 3;j++){
            if( areThereMissingMessages(&(mon_p->msgChk)) ){
                LOG(LOG_INFO, "seems like there are missing messages");
                getMissingMessages(mon_p, scratchpad);
            }else{
                LOG(LOG_INFO,"........................");
                LOG(LOG_INFO,"allright, ALL Messages are HERE");
                LOG(LOG_INFO,"........................");
                break;
            }
        }


        sleep(20);

    } // while(1)

    return NULL;
}



void *serial_f(void *arg)
{
    struct monitor_s *mon_p = (struct monitor_s *)arg;
    struct ubx_hdr *ubxMsg_p = NULL;
    uint8_t uartRxBuf[UART_RX_BUF_SIZE];
    uint8_t pauseBtwSend = 0;
    int payloadLen = 0;
    int r,t;

    memset(uartRxBuf, 0, sizeof(uint8_t) * UART_RX_BUF_SIZE);

    while(1){

        //if( mon_p->msgChk->ubxComEnable ){
        if( 1 ){

            if( (0 < size(mon_p->llistTxCommands)) && (pauseBtwSend >= 2)  ){
                ubxMsg_p = front(mon_p->llistTxCommands);
                payloadLen = getUbx_MsgLength(ubxMsg_p);

                // send ubx message via serial port, note that +2 is due to acknowledge bytes
                t = write(mon_p->serialPort_p->fd,(uint8_t *) ubxMsg_p, sizeof(struct ubx_hdr) + payloadLen + 2);
                if(t == sizeof(struct ubx_hdr) + payloadLen + 2){
                    // get rid of the processed node
                    remove_front(mon_p->llistTxCommands, freeNode);
                }
                pauseBtwSend = 0;
            }
            pauseBtwSend += 1;
        }

        r = read(mon_p->serialPort_p->fd, uartRxBuf, 100);
        if(0 < r){
            ringbuffer_write(mon_p->rbUartIn_p, uartRxBuf, r);
            mon_p->serialPort_p->ticksBtwReceive = 0;

            printf("incoming %d bytes: ", r);
            for(int ii=0; ii < r; ii++){ 
                printf("%02X", uartRxBuf[ii]);
            }
            printf("\n");

            memset(uartRxBuf, 0, sizeof(uint8_t)*UART_RX_BUF_SIZE);

        }else{
            mon_p->serialPort_p->ticksBtwReceive += 1;
        }

    } // end of while(1)

}
    

int main(int argc, char *argv[])
{

    struct monitor_s *mon_p = NULL;
    pthread_t idThreadSerial[2]; // wr, rd


    mon_p = prep_monitoringStruct();
    setDbgLogs();
    
    pthread_create(&idThreadSerial[0], NULL, control_f, (void *)mon_p);
    pthread_create(&idThreadSerial[1], NULL, serial_f,  (void *)mon_p);

    pthread_join(idThreadSerial[0], NULL);
    pthread_join(idThreadSerial[1], NULL);

    return 0;
}


