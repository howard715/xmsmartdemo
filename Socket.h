#ifndef __GAGENT_SOCKET_H_
#define __GAGENT_SOCKET_H_

#include "xmplatform.h"
#include "xmsocket.h"
//#define SOCKET_RECBUFFER_LEN (1*1024)
//#define SOCKET_TCPSOCKET_BUFFERSIZE    1*1024
#define MAX_SOCKET_SEND_LEN	2048

#ifndef SOCK_TAG
#define SOCK_TAG

#define CONNECT_DEV_PASS_RIGHT  3
#define CONNECT_DEV_PASS_ERROR  4
#define DEVICE_STATE_LANONLINE	6
#define DEVICE_MSG_RETURN	7
#define DEVICE_MSG_PUBLISH	8
#define DEVICE_STATE_WANONLINE  101
#define DEVICE_CHANGEPASS_OK	104
#define DEVICE_CHANGEPASS_ERROR 105

//设备接收
#define SOCKET_DISCOVER 0x01
#define SOCKET_TICKET	0x02
#define SOCKET_CHECKPASS	0x03
#define SOCKET_MSG_PUT	0x04
#define SOCKET_CHANGEPASS 0x05
#define SOCKET_DISCOVER_SN 0x06
//返回给app
#define SOCKET_PASS_RIGHT	CONNECT_DEV_PASS_RIGHT
#define SOCKET_PASS_ERROR	CONNECT_DEV_PASS_ERROR
#define SOCKET_CHANGEPASS_OK	DEVICE_CHANGEPASS_OK
#define SOCKET_CHANGEPASS_ERROR DEVICE_CHANGEPASS_ERROR
#define SOCKET_TICKETOK DEVICE_STATE_LANONLINE
#define CLOUD_SOCKET_TICKETOK	DEVICE_STATE_WANONLINE

#endif

void Socket_CreateTCPServer(int tcp_port);
void Socket_CreateUDPBroadCastServer( int udp_port );
void Socket_SendPubMsg(uint8 *uart_buf,int msglen);
void handlePasscode(xm_soc_t *xm_soc,uint8 *buf,int buflen);
void broad_recv_callback(void *param);
void lan_data_recv(void *param);
void tcp_server_accept(void *param);
void change_devpass(xm_soc_t* client,uint8* data);
#endif
