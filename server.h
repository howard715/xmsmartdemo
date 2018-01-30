#ifndef __SERVER_H_
#define __SERVER_H_
#include "xmplatform.h"
#include "xmqueue.h"

#define SDK_VERSION "v2.1"

extern uint32 cloud_server_ip;
extern xm_conf *xcf;

int ch2int(char ch);
void BuildDataHeader(uint8* SendPack,uint8 cmd,int datalen,int handle);
int parse_content_len(char *pdata);
void xm_event_call(event_cmd event,void *param);


#endif
