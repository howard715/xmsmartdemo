#ifndef _XMSOCKET_H
#define _XMSOCKET_H
#include "xmplatform.h"
#define UDP_LISTEN_TYPE 0x01
#define TCP_LISTEN_TYPE 0x02
#define CLIENT_TYPE 0x03
#define CLOUD_TYPE 0x04
#define HTTPLOG_TYPE 0x05
#define MAX_SOCKET_AVL 6

typedef enum _SOCKET_STREAM_TYPE{
    SOCKET_STREAM_TCP,
    SOCKET_STREAM_UDP
}SOCKET_STREAM_TYPE;

xm_soc_t *xm_create_socket(uint8 soc_type,SOCKET_STREAM_TYPE stream_type);
bool xm_socket_bind(xm_soc_t *xm_soc,int bind_port);
xm_soc_t* xm_socket_accept(int sock_fd);
bool xm_socket_listen(xm_soc_t *xm_soc);

bool xm_socket_connect(xm_soc_t *xm_soc,uint16 remote_port,uint32 remote_ip);
bool xm_socket_add(xm_soc_t *xm_soc);
int xm_socket_send(xm_soc_t *xm_soc,const uint8 *data,uint16 len);
void xm_socket_sendto(xm_soc_t* xm_soc,void *udpaddr,const uint8 *data,uint16 len);

void xm_socket_Lsendpub(uint8 *data ,uint16 len);
bool xm_sock_read_valide(xm_soc_t *xm_soc);
void xm_socket_init();
xm_soc_t* xm_socket_get_cloud();
#endif
