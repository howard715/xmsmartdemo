#include "xmplatform.h"
#include "Socket.h"
#include "xmqueue.h"
#include "server.h"
#include "xmsocket.h"
#include "mqttlib.h"
#include "mqttxpg.h"
#include "xmtimer.h"

void handlePasscode(xm_soc_t *xm_soc,uint8 *buf,int buflen)
{
	if(strlen(buf) != (strlen(xcf->snconf.Cloud_SN)+strlen(xcf->snconf.SN_Pass)))
		goto error;
	if(memcmp(buf,xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN))!=0)
		goto error;
	if(memcmp(&buf[strlen(xcf->snconf.Cloud_SN)],xcf->snconf.SN_Pass,strlen(xcf->snconf.SN_Pass))!=0)
		goto error;

	_xm_SendData2APP(xm_soc,NULL,0,SOCKET_PASS_RIGHT);
	return;
error:
	_xm_SendData2APP(xm_soc,NULL,0,SOCKET_PASS_ERROR);
}

bool devpass_json_parse(uint8 *data)
{
	cJSON *json_ptr=NULL,*psubitem=NULL;
	char sn[40]={0};
	char newpass[20]={0},oldpass[20]={0};
	json_ptr=cJSON_Parse(data);

	if(json_ptr==NULL)
		return false;

	psubitem=cJSON_GetObjectItem(json_ptr,"sn");
	if(psubitem)
	{
		strcpy(sn,psubitem->valuestring);
	}else
		goto error;
	psubitem=cJSON_GetObjectItem(json_ptr,"old_pwd");
	if(psubitem)
	{
		strcpy(oldpass,psubitem->valuestring);
	}else
		goto error;

	psubitem=cJSON_GetObjectItem(json_ptr,"new_pwd");
	if(psubitem)
	{
		strcpy(newpass,psubitem->valuestring);
	}else
		goto error;
success:
	if(strcmp(sn,xcf->snconf.Cloud_SN)==0
	&& strcmp(oldpass,xcf->snconf.SN_Pass)==0)
	{
		memcpy(xcf->snconf.SN_Pass, newpass, strlen(newpass));
	}else
		goto error;
	cJSON_Delete(json_ptr);
	return true;
error:
	cJSON_Delete(json_ptr);
	return false;
}

void change_devpass(xm_soc_t* client,uint8* data)
{
	bool ret;

	ret = devpass_json_parse(data);

	if(ret)
	{
		_xm_SendData2APP(client,NULL,0,SOCKET_CHANGEPASS_OK);
		if(xcf->callback)
			xcf->callback(client,NULL,0,MSG_PASS_CHANGE);
	}else{
		_xm_SendData2APP(client,NULL,0,SOCKET_CHANGEPASS_ERROR);
	}
}
void broad_recv_callback(void *param)
{
	os_handle_t *xm_os_handle = (os_handle_t*)param;
	uint8 *pdata = xm_os_handle->pdata;
	uint16 len = xm_os_handle->data_len;
	xm_soc_t *xm_soc = xm_os_handle->xm_soc;

	char devinfo[100]={0};
	if(pdata == NULL)
		return;
	if (len >= 4 && pdata[0]==0x00 && pdata[1]==0x00 && pdata[2]==0x00){

		sprintf(devinfo,"{\"SN\":\"%s\",\"MAC\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"IPADD\":\"%s\",\"TYPE\":\"01\"}",xcf->snconf.Cloud_SN,
				xcf->mac[0],xcf->mac[1],xcf->mac[2],xcf->mac[3],xcf->mac[4],xcf->mac[5],xcf->ipadd);

		if(pdata[3]==SOCKET_DISCOVER) //Ondiscover
		{
			xm_socket_sendto(xm_soc,xm_os_handle->udpaddr,devinfo,strlen(devinfo));
		}
		else if(pdata[3]==SOCKET_DISCOVER_SN) //Ondiscover
		{
			if(strcmp(&pdata[4],xcf->snconf.Cloud_SN)==0){
				xm_socket_sendto(xm_soc,xm_os_handle->udpaddr,devinfo,strlen(devinfo));
			}
		}
	}

}

void Socket_CreateUDPBroadCastServer( int udp_port )
{
	xm_soc_t *xm_soc=NULL;
	xm_soc = xm_create_socket(UDP_LISTEN_TYPE,SOCKET_STREAM_UDP);

	if(xm_soc == NULL){
		xm_debug("xm_soc null\n");
		return;
	}
	if(xm_socket_bind(xm_soc,udp_port) == false)
	{
		xm_debug("bind error\n");
		return;
	}
	xm_debug("udp band\n");
}

void LanAppTick_Ack( xm_soc_t* conn ,char *data)
{
	int datalen = strlen(data);
	if(datalen != (strlen(xcf->snconf.Cloud_SN)+strlen(xcf->snconf.SN_Pass)))
		goto error;

	if(memcmp(data,xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN))!=0)
		goto error;
	if(memcmp(&data[strlen(xcf->snconf.Cloud_SN)],xcf->snconf.SN_Pass,strlen(xcf->snconf.SN_Pass))!=0)
		goto error;

	_xm_SendData2APP(conn,NULL,0,SOCKET_TICKETOK);
	return;
error:
	_xm_SendData2APP(conn,NULL,0,SOCKET_PASS_ERROR);
}


void lan_data_recv(void* param)
{
	os_handle_t *xm_os_handle = (os_handle_t*)param;
	uint8 *pdata = xm_os_handle->pdata;
	uint16 len = xm_os_handle->data_len;
	xm_soc_t *xm_soc = xm_os_handle->xm_soc;

	memset(xm_soc->clientid,0,sizeof(xm_soc->clientid));

	if(len<5 || pdata == NULL)
		return;
	xm_soc->nowtiming = xm_timestamp();
	xm_soc->handle = pdata[4];
	if(pdata[0]==0x00
	&& pdata[1]==0x00
	&& pdata[2]==0x00
	&& pdata[3]==SOCKET_CHECKPASS
	  )
	{
		handlePasscode( xm_soc,&pdata[5], len-5);
		return;
	}
	//if(Socket_Get_LoginStatu(nSocket) != 1)
	//	return;
	if(pdata[0]==0x00&&pdata[1]==0x00&&pdata[2]==0x00)
	{
		switch(pdata[3])
		{
			case SOCKET_TICKET:
				LanAppTick_Ack(xm_soc,&pdata[5]);
				break;
			case SOCKET_MSG_PUT:
				if(xcf->callback)
					xcf->callback(xm_soc,&pdata[5],len-5,MSG_CONTROL);
				break;
			case SOCKET_CHANGEPASS:
				change_devpass(xm_soc,&pdata[5]);
				break;
			default:
				break;
		}
	}
}

void tcp_server_accept(void *param)
{
	os_handle_t *xm_os_handle = (os_handle_t*)param;
	xm_soc_t *xm_soc = xm_os_handle->xm_soc;

	xm_soc_t *n_xm_soc=NULL;
	n_xm_soc = xm_socket_accept(xm_soc->fd);
	if (n_xm_soc)
	{
		xm_debug("new client %d\n",n_xm_soc->fd);
	}

}
void Socket_CreateTCPServer(int tcp_port)
{
	xm_soc_t *xm_soc=NULL;
	xm_soc = xm_create_socket(TCP_LISTEN_TYPE,SOCKET_STREAM_TCP);

	if(xm_soc == NULL){
		xm_debug("xm_soc null\n");
		return;
	}
	if(xm_socket_bind(xm_soc,tcp_port) == false)
	{
		xm_debug("bind error\n");
		return;
	}

	if(xm_socket_listen(xm_soc)==false)
	{
		xm_debug("listen error\n");
		return;
	}

	xm_debug("start listen\n");
}
