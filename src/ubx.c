/*
 * ubx.c
 *
 * Implementation of generic UBX helpers
 *
 *
 * Copyright (C) 2009  Sylvain Munaut <tnt@246tNt.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "ubx.h"
#include "list.h"
#include "debug.h"

//Private prototype additions
static void *genPollMessageIndSV(uint8_t msg_class, uint8_t msg_id, uint8_t svid);
static void *genPollMessage(uint8_t msg_class, uint8_t msg_id);
static void *genNmeaSilencer_msg_1(void);
static void *genNmeaSilencer_msg_2(void);
static void *genNmeaSilencer_msg_3(void);
static void *genNmeaSilencer_msg_3(void);
static void *genNmeaSilencer_msg_4(void);
static void *genNmeaSilencer_msg_5(void);
static void *genNmeaSilencer_msg_6(void);
static void *genNmeaSilencer_msg_7(void);
// --------------------------


static void ubx_checksum(uint8_t *data, int len, uint8_t *cksum);
static ubx_msg_handler_t ubx_find_handler(struct ubx_dispatch_entry *dt, uint8_t msg_class, uint8_t msg_id);



static void ubx_checksum(uint8_t *data, int len, uint8_t *cksum)
{
	int i;
	uint8_t ck0 = 0, ck1 = 0;
	for (i=0; i<len; i++) {
		ck0 += data[i];
		ck1 += ck0;
	}
	cksum[0] = ck0;
	cksum[1] = ck1;
}


static ubx_msg_handler_t ubx_find_handler(struct ubx_dispatch_entry *dt, uint8_t msg_class, uint8_t msg_id)
{
	while (dt->handler) {
		if ((dt->msg_class == msg_class) && (dt->msg_id == msg_id))
			return dt->handler;
		dt++;
	}
	return NULL;
}


int ubx_msg_dispatch(struct ubx_dispatch_entry *dt, void *msg, int len, void *userdata)
{
	struct ubx_hdr *hdr = msg;
	uint8_t cksum[2], *cksum_ptr;
	ubx_msg_handler_t h;

	if ((hdr->sync[0] != UBX_SYNC0) || (hdr->sync[1] != UBX_SYNC1)) {
		LOG(LOG_ERR, "[!] Invalid sync bytes\n");
		return -1;
	}

	ubx_checksum(msg + 2, sizeof(struct ubx_hdr) + hdr->payload_len - 2, cksum);
	cksum_ptr = msg + (sizeof(struct ubx_hdr) + hdr->payload_len);
	if ((cksum_ptr[0] != cksum[0]) || (cksum_ptr[1] != cksum[1])) {
		LOG(LOG_ERR, "[!] Invalid checksum\n");
		return -1;
	}

	h = ubx_find_handler(dt, hdr->msg_class, hdr->msg_id);
	if (h){
        LOG(LOG_INFO, "found handler");
		h(hdr, msg + sizeof(struct ubx_hdr), hdr->payload_len, userdata);
    }

	return sizeof(struct ubx_hdr) + hdr->payload_len + 2;
}

//////////////////////////////////////////////////////////////////////////////
// AIDING DATA POLL MESSAGES
/////////////////////////////////////////////////////////////////////////////



// Poll Message used to request for all SV
static void * genPollMessage(uint8_t msg_class, uint8_t msg_id)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t)*8);
    if(NULL == message){
        return NULL;
    }

    message[0]= UBX_SYNC0;   // sync 0
    message[1]= UBX_SYNC1;   // sync 1
    message[2]= msg_class;
    message[3]= msg_id;
    message[4]= 0;           // length 1
    message[5]= 0;           // length 2
    message[6]= 0;           // checksum 1
    message[7]= 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) - 2, cksum);

    message[6] = cksum[0];
    message[7] = cksum[1];

    return (void *)message;

}



// Poll Message used to request for a single SV
static void * genPollMessageIndSV(uint8_t msg_class, uint8_t msg_id, uint8_t svid)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t)*9);
    if(NULL == message){
        return NULL;
    }

    message[0]= UBX_SYNC0;   // sync 0
    message[1]= UBX_SYNC1;   // sync 1
    message[2]= msg_class;
    message[3]= msg_id;
    message[4]= 1;           // length 1
    message[5]= 0;           // length 2
    message[6]= svid;        // payload
    message[7]= 0;           // checksum 1
    message[8]= 0;           // checksum 2


    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+1 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 1 - 2, cksum);

    message[7] = cksum[0];
    message[8] = cksum[1];

    return (void *)message;
}

//TODO: NOte that there is simply the UBX_AID_DATA Message
//      which polls ubx_aid_ini, hui, alm, eph signals

void * pollAlmanac(int svid)
{
    if(-1 == svid){
        return genPollMessage(UBX_CLASS_AID, UBX_AID_ALM);
    }else if(0 <= svid){
        return genPollMessageIndSV(UBX_CLASS_AID, UBX_AID_ALM, svid);
    }else{
        return NULL;
    }
}

void * pollEphem(int svid)
{
    if(-1 == svid){
        return genPollMessage( UBX_CLASS_AID, UBX_AID_EPH);
    }else if(0 <= svid){
        return genPollMessageIndSV( UBX_CLASS_AID, UBX_AID_EPH, svid);
    }else{
        return NULL;
    }
}

void * pollHui(void)
{
    return genPollMessage(UBX_CLASS_AID, UBX_AID_HUI);
}

void * pollIni(void)
{
    return genPollMessage(UBX_CLASS_AID, UBX_AID_INI);
}

void * pollPosllh(void)
{
    return genPollMessage(UBX_CLASS_NAV, UBX_NAV_POSLLH);
}


int getUbx_MsgLength(void *msg)
{
    struct ubx_hdr *ubx = (struct ubx_hdr *)msg;


    if(NULL == ubx){
        LOG(LOG_INFO, "error in len");
        return 0;
    }else{
        return ubx->payload_len;
    }


    uint8_t *ptr = (uint8_t *)msg;
    return (((uint16_t) *(ptr+5)) << 8) + ((uint16_t) *(ptr+4));
}

int getUbx_MsgClass(void *msg)
{
    struct ubx_hdr *ubx = (struct ubx_hdr *)msg;

    return ubx->msg_class;
}

int getUbx_MsgId(void *msg)
{
    struct ubx_hdr *ubx = (struct ubx_hdr *)msg;

    return ubx->msg_id;
}

int getUbx_SVID(uint8_t *msg)
{
    uint8_t svid;
    svid = *(msg + 6);
    if(svid > 32){
        svid = 33;  // 33 corresponds to almBuf, ephBuf
    }
    return svid;
}

int prepAidMissingPollMsgs(struct llist *ll, struct msgStrmCheck_s *msgChk)
{
    void *ubxMsg_p = NULL;


    if(0 == msgChk->ubxAidAck.iniAck){
        ubxMsg_p = pollIni();
        if(NULL == ubxMsg_p){ LOG(LOG_ERR, "null pointer ini!!"); }
        push_back(ll, (void *)ubxMsg_p);
    }

    if(0 == msgChk->ubxAidAck.huiAck){
        ubxMsg_p = pollHui();
        if(NULL == ubxMsg_p){ LOG(LOG_ERR, "null pointer hui!!"); }
        push_back(ll, (void *)ubxMsg_p);
    }

    if(0 == msgChk->ubxAidAck.posllhAck){
        ubxMsg_p = pollPosllh();
        if(NULL == ubxMsg_p){ LOG(LOG_ERR, "null pointer posllh!!"); }
        push_back(ll, (void *)ubxMsg_p);
    }

    for(int i=0; i<32; i++)
    {
        if(0 == msgChk->ubxAidAck.almAck[i] ){
            ubxMsg_p = pollAlmanac(i);
            if(NULL == ubxMsg_p){ LOG(LOG_ERR, "null pointer alm!!"); }
            push_back(ll, (void *)ubxMsg_p);
        }
    }

    for(int i=0; i<32; i++)
    {
        if(0 == msgChk->ubxAidAck.ephAck[i] ){
            ubxMsg_p = pollEphem(i);
            if(NULL == ubxMsg_p){ LOG(LOG_ERR, "null pointer eph!!"); }
            push_back(ll, (void *)ubxMsg_p);
        }
    }

    return 0;

}


int areThereMissingMessages(struct msgStrmCheck_s *msgChk)
{
    int m = 0;

    for(int i=0; i<32; i++)
    {
        if(0 == msgChk->ubxAidAck.almAck[i] ){
            LOG(LOG_WARN, "missing almanac svid:%d", i);
            m++;
        }
    }

    for(int i=0; i<32; i++)
    {
        if(0 == msgChk->ubxAidAck.ephAck[i] ){
            LOG(LOG_WARN, "missing ephemeris svid:%d", i);
            m++;
        }
    }

    if(0 == msgChk->ubxAidAck.iniAck){
        LOG(LOG_WARN, "missing ini");
        m++;
    }

    if(0 == msgChk->ubxAidAck.huiAck){
        LOG(LOG_WARN, "missing hui");
        m++;
    }

    if(0 == msgChk->ubxAidAck.posllhAck){
        LOG(LOG_WARN, "missing posllh");
        m++;
    }

    return m;
}




int prepAidPollMsgs(struct llist *ll)
{
    void *ubxMsg_p = NULL;

    ubxMsg_p = pollHui();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = pollIni();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = pollAlmanac(-1);
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = pollEphem(-1);
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = pollPosllh();
    push_back(ll, (void *)ubxMsg_p);

    return 0;

}


int prepNmeaSilencerMsgs(struct llist *ll)
{
    void *ubxMsg_p = NULL;

    ubxMsg_p = genNmeaSilencer_msg_1();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = genNmeaSilencer_msg_2();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = genNmeaSilencer_msg_3();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = genNmeaSilencer_msg_4();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = genNmeaSilencer_msg_5();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = genNmeaSilencer_msg_6();
    push_back(ll, (void *)ubxMsg_p);
    ubxMsg_p = genNmeaSilencer_msg_7();
    push_back(ll, (void *)ubxMsg_p);

    return 7;
}


// check whether we have valid ubx message in incoming uart data
int parseUartInput_4_UbxMsg(void *msg, int bytesLeftInBuffer)
{
    struct ubx_hdr *hdr = msg;
    uint8_t cksum[2], *cksum_ptr;

    if( (hdr->sync[0] != UBX_SYNC0) || (hdr->sync[1] != UBX_SYNC1) ){

        //LOG(LOG_ERR, "[!] Invalid sync bytes\n");
        return -1;
    }

    hdr->payload_len = getUbx_MsgLength(msg);
    // Check whether we have enough bytes left in buffer to reach out
    // for cksum bytes
    if( bytesLeftInBuffer < sizeof(struct ubx_hdr) + hdr->payload_len + 2){
        return -2;
    }

    ubx_checksum(msg + 2, sizeof(struct ubx_hdr) + hdr->payload_len - 2, cksum);
    cksum_ptr = msg + (sizeof(struct ubx_hdr) + hdr->payload_len);
	if ((cksum_ptr[0] != cksum[0]) || (cksum_ptr[1] != cksum[1])) {
        LOG(LOG_INFO, "[!] Invalid checksum\n");
		return -3;
	}

	return sizeof(struct ubx_hdr) + hdr->payload_len + 2;
}

int updateValidUbxMsgList(void *ptr, struct msgStrmCheck_s *msgChk)
{
    struct ubx_hdr *ubxMsg = (struct ubx_hdr *)ptr;
    int class, id, svid;

    //LOG(LOG_WARN, "updating valid ubx msg list now");
    class = getUbx_MsgClass( (void *)ubxMsg);
    id    = getUbx_MsgId(    (void *)ubxMsg);
    svid  = getUbx_SVID( (uint8_t *)ubxMsg );

    if( UBX_CLASS_AID == class) {

        switch (id) {

        case UBX_AID_INI :


            LOG(LOG_INFO, "aid ini okey");
            msgChk->ubxAidAck.iniAck = 1;
            break;

        case UBX_AID_HUI:

            LOG(LOG_INFO, "aid hui okey");
            msgChk->ubxAidAck.huiAck = 1;
            break;

        case UBX_AID_ALM:

            LOG(LOG_INFO, "aid alm sat:%d okey",svid);
            msgChk->ubxAidAck.almAck[svid] = 1;
            break;

        case UBX_AID_EPH:

            LOG(LOG_INFO, "aid eph sat:%d okey",svid);
            msgChk->ubxAidAck.ephAck[svid] = 1;
            break;

        default:
            break;
        }

    }else if( UBX_CLASS_NAV == class){

        if( UBX_NAV_POSLLH == id){
            LOG(LOG_INFO, "nav posllh okey");
            msgChk->ubxAidAck.posllhAck = 1;
        }
    }

    return 0;

} // updateValidUbxMsgList(
            




    



static void * genNmeaSilencer_msg_1(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x00;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}

static void * genNmeaSilencer_msg_2(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x01;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}


static void * genNmeaSilencer_msg_3(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x02;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}


static void * genNmeaSilencer_msg_4(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x03;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}


static void * genNmeaSilencer_msg_5(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x04;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}

static void * genNmeaSilencer_msg_6(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x05;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}

static void * genNmeaSilencer_msg_7(void)
{
    uint8_t *message = NULL;
    uint8_t cksum[2];

    message = (uint8_t *)malloc( sizeof(uint8_t) * 11);
    if(NULL == message){
        return NULL;
    }

    message[0]  = UBX_SYNC0;
    message[1]  = UBX_SYNC1;
    message[2]  = UBX_CLASS_CFG;
    message[3]  = UBX_CFG_MSG;
    message[4]  = 3;           // length 1
    message[5]  = 0;           // length 2
    message[6]  = 0xF0;        // payload
    message[7]  = 0x08;        // payload 
    message[8]  = 0x00;        // payload
    message[9]  = 0;           // checksum 1
    message[10] = 0;           // checksum 2

    //Checksum calculation starts from class_id and ends with payload
    //-2 is there to compansate for the uint8_t sync[2]
    //+3 is there to compansate for payload length
    // hence we have sizeof(struct ubx_hdr)-2 = 6-2 = 4
	ubx_checksum(message + 2, sizeof(struct ubx_hdr) + 3 - 2, cksum);

    message[9]  = cksum[0];
    message[10] = cksum[1];

    return (void *)message;
}

