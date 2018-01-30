#include "xmplatform.h"
#include "httplog.h"
#include "xmqueue.h"
#include "Socket.h"
#include "server.h"
#include "mqttxpg.h"

uint32 cloud_server_ip =0;
xm_conf *xcf = NULL;

void GetCloudDns();
void* xm_zalloc(int size){ 
	do {void *ptr=NULL;ptr = malloc(size);if(ptr)memset(ptr,0,size);return (void*)ptr;}while(0);
}
int ch2int(char ch)
{
	int ret = 0;
	switch(ch)
	{
		case '0' ... '9':
			ret = ch - '0';
			break;
		case 'a' ... 'z':
			ret = ch-'z'+10;
			break;
		case 'A' ... 'Z':
			ret = ch-'A'+10;
			break;
	}
	return ret;
}
int parse_content_len(char *pdata)
{
	char *start=NULL;
	char *end=NULL;
	char tmplen[20]={0};
	start = (char*)strstr(pdata,"Content-Length: ");
	if(start==NULL)
		return 0;

	start += 16;
	end = (char*)strstr(start,"\r\n");
	if(end == NULL)
		return 0;

	memcpy(tmplen,start,(end-start));
	return atoi(tmplen);
}
void BuildDataHeader(uint8 *SendPack,uint8 cmd,int msglen,int handle)
{
	int total_len;
	total_len=msglen+4+1;
	if (total_len <= 127) {
		SendPack[1] = total_len;
	} else {
		SendPack[1] = total_len % 128;
		SendPack[1] = SendPack[1] | 0x80;
		SendPack[2] = total_len / 128;
	}
	SendPack[6]=cmd;
	SendPack[7]=handle;
}

void SendPublishData(char *data,int len)
{
	xm_SendPublishPacket(data,len);
	xm_socket_Lsendpub(data,len);
}
void sock_msg_call(void *param)
{
	os_handle_t *os_handle = (os_handle_t*)param;
	xm_soc_t *xm_soc = os_handle->xm_soc;
	switch(xm_soc->soc_type)
	{
		case UDP_LISTEN_TYPE:
			broad_recv_callback(os_handle);
			break;
		case TCP_LISTEN_TYPE:
			tcp_server_accept(os_handle);
			break;
		case CLIENT_TYPE:
			lan_data_recv(os_handle);
			break;
		case CLOUD_TYPE:
			cloud_data_recv(os_handle);
			break;
		case HTTPLOG_TYPE:
			http_Receive(os_handle);
			break;
		default:
			break;
	}
	if(os_handle->pdata) free(os_handle->pdata);
	if(os_handle->udpaddr) free(os_handle->udpaddr);
	free(os_handle);
}
void xm_event_call(event_cmd event,void *param)
{
	switch (event) {
		case HTTPLOG_EVENT_WARINGLOG:
			Http_SendDataServer(param,0);
			break;
		case CLOUD_CONNECT:
			xm_debug("connect cloud\n");
			Connect_cloud();
			break;
		case DNS_FOUND:
			GetCloudDns();
			break;
		case SOCK_MSG_EVENT:
			sock_msg_call(param);
			break;
		default:
			break;
	}
}
void SendWebLogData(char* logmsg)
{
	xm_sendqueue_event(xm_event_call,HTTPLOG_EVENT_WARINGLOG,(void*)logmsg);
}
void GetCloudDns()
{
	struct hostent *ipAddress;

	if ((ipAddress = gethostbyname("api.ximotech.com")) == 0)
	{
		xm_sendqueue_event(xm_event_call,DNS_FOUND,0);
		return;
	}
	cloud_server_ip = ((struct in_addr*)(ipAddress->h_addr))->s_addr;
	xm_sendqueue_event(xm_event_call,CLOUD_CONNECT,0);
}
void xm_cloud_init(xm_conf *conf)
{
	xcf = conf;
	xm_event_queue_init();
	xm_socket_init();

	Socket_CreateTCPServer(12416);
	Socket_CreateUDPBroadCastServer(12415);
	xm_sendqueue_event(xm_event_call,DNS_FOUND,0);
	http_register_dev();
} 
