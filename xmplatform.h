#ifndef _XM_PLATFORM_H
#define _XM_PLATFORM_H

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <netdb.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdarg.h>
#include <syslog.h>
#include <fcntl.h>
#include <termios.h>    /* POSIX terminal control definitions */
#include <sys/types.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <malloc.h>
#include "cJSON.h"

#ifndef bool
#define bool int
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif
#ifndef uint8
#define uint8 unsigned char
#endif
#ifndef uint32
#define uint32 unsigned int
#endif
#ifndef uint64
#define uint64 unsigned long long
#endif

#ifndef uint16
#define uint16 unsigned short
#endif


#define XM_DID_LEN_MAX          40
#define XM_PASS_LEN             22

#define MSG_PASS_CHANGE	0x01
#define MSG_CONTROL	0x02

#define PRODUCTKEY "11f511705b39ef7d00c8f86c33cd68ac"
#define APPID "8617518c11befb13c3564669bf629ca9"
#ifndef zalloc
#define zalloc xm_zalloc
#endif

#define XMCLOUDLOG
#ifdef XMCLOUDLOG
#define xm_debug(...) printf("Filename %s, Function %s, Line %d > ", __FILE__, __FUNCTION__, __LINE__); \
	printf(__VA_ARGS__);
#else
#define xm_debug(...)
#endif

struct _xm_soc_s{
    int fd;
    uint8 *pdata;
    uint16 data_len;
    char clientid[40];
    int handle;
    uint8 soc_type;
    bool soc_valide;
    void *udpaddr;
    uint32 seq;
    uint64 nowtiming;
};
extern void* xm_zalloc(int size);
typedef struct _xm_soc_s xm_soc_t;
typedef struct _SN_CONFIG {
	char SN_Pass[XM_PASS_LEN];
	char Cloud_SN[XM_DID_LEN_MAX];
}__attribute__ ((aligned (4))) SN_CONFIG;

typedef void (*xm_msg_callback)(xm_soc_t *,uint8*,uint16,uint8 msg_type);

typedef struct _xm_conf_{
	SN_CONFIG snconf;
	uint8 mac[6];
	char ipadd[20];
	xm_msg_callback callback;
}xm_conf;

extern void xm_SendData2Dev(uint8 *uartbuf,int uartbufLen,uint8 *devsn);
extern void xm_SendData2APP(xm_soc_t *xm_soc,uint8 *uartbuf,int uartbufLen);
extern void SendPublishData(char *data,int len);
extern void SendWebLogData(char* logmsg);
extern void xm_cloud_init(xm_conf *conf);

#endif
