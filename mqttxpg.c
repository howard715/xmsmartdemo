#include "xmplatform.h"
#include "mqttxpg.h"
#include "Socket.h"
#include "mqttlib.h"
#include "server.h"
#include "xmtimer.h"

uint16_t g_Msgsub_id=0;
int g_MQTTStatus = MQTT_STATUS_LOGIN;

void init_pubmsg_head(uint8 *McuSendPubTopic)
{
	memcpy( McuSendPubTopic,"devpub/",strlen("devpub/"));
	memcpy( McuSendPubTopic+strlen("devpub/"),xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN));
	McuSendPubTopic[strlen("devpub/")+strlen(xcf->snconf.Cloud_SN)]='/';
	memcpy(&McuSendPubTopic[strlen("devpub/")+strlen(xcf->snconf.Cloud_SN)+1],xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN));
	McuSendPubTopic[strlen("devpub/")+strlen(xcf->snconf.Cloud_SN)+1+strlen(xcf->snconf.Cloud_SN)]='\0';
}
void xm_SendPublishPacket(uint8 *uartbuf,int uartbufLen)
{
	uint8 McuSendPubTopic[200] ={0};
	uint8 head[8]={0};
	xm_soc_t *xm_soc=NULL;
	xm_soc = xm_socket_get_cloud();
	if(xm_soc == NULL)
		return;
	BuildDataHeader(head,DEVICE_MSG_PUBLISH,uartbufLen,0);
	init_pubmsg_head(McuSendPubTopic);
	XPGmqtt_publish(xm_soc,McuSendPubTopic,uartbuf,uartbufLen,0,head,8);
}
int _xm_SendData2APP(xm_soc_t* xm_soc,uint8 *uartbuf,int uartbufLen,uint8 cmd)
{
	uint8 McuSendTopic[200]={0};
	uint8 head_msg[13]={0x00};
	int head_msg_len = 8;
	sprintf(&head_msg[8],"%s",SDK_VERSION);
	if(cmd == SOCKET_PASS_ERROR || cmd == CLOUD_SOCKET_TICKETOK || cmd == SOCKET_TICKETOK)
	{
		BuildDataHeader(head_msg,cmd,uartbufLen+4,xm_soc->handle);//4个字节的sdk版本
		head_msg_len = 12;
	}
	else
	{
		BuildDataHeader(head_msg,cmd,uartbufLen,xm_soc->handle);
	}
	if(xm_soc->clientid[0]!= 0)
	{
		memcpy( McuSendTopic,"dev2app/",strlen("dev2app/"));
		memcpy( McuSendTopic+strlen("dev2app/"),xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN));
		McuSendTopic[strlen("dev2app/")+strlen(xcf->snconf.Cloud_SN)]='/';
		memcpy( McuSendTopic+strlen("dev2app/")+strlen(xcf->snconf.Cloud_SN)+1,xm_soc->clientid,strlen(xm_soc->clientid));
		McuSendTopic[strlen("dev2app/")+strlen(xcf->snconf.Cloud_SN)+1+strlen(xm_soc->clientid)]='\0';
		XPGmqtt_publish(xm_soc,McuSendTopic,uartbuf,uartbufLen,0,head_msg,head_msg_len);
	}else
	{
		xm_socket_send(xm_soc,head_msg,head_msg_len);
		if(uartbuf)
			xm_socket_send(xm_soc,uartbuf,uartbufLen);
	}
	return 0;
}

void xm_SendData2Dev(uint8 *uartbuf,int uartbufLen,uint8 *devsn)
{
	uint8 McuSendTopic[200]={0};
	uint8 head_msg[8]={0};
	xm_soc_t *xm_soc=NULL;
	xm_soc = xm_socket_get_cloud();
	if(xm_soc == NULL)
		return;
	BuildDataHeader(head_msg,SOCKET_MSG_PUT,uartbufLen,0);

	memcpy( McuSendTopic,"app2dev/",strlen("app2dev/"));
	memcpy( McuSendTopic+strlen("app2dev/"),devsn,strlen(devsn));
	McuSendTopic[strlen("app2dev/")+strlen(devsn)]='/';
	memcpy( McuSendTopic+strlen("app2dev/")+strlen(devsn)+1,xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN));
	McuSendTopic[strlen("app2dev/")+strlen(devsn)+1+strlen(xcf->snconf.Cloud_SN)]='\0';
	XPGmqtt_publish(xm_soc,McuSendTopic,uartbuf,uartbufLen,0,head_msg,8);
}

void xm_SendData2APP(xm_soc_t *xm_soc,uint8 *uartbuf,int uartbufLen)
{
	_xm_SendData2APP(xm_soc,uartbuf,uartbufLen,DEVICE_MSG_RETURN);
}

void MQTT_DoCloudMCUCmd(xm_soc_t *xm_soc,uint8 *pHiP0Data, int P0DataLen)
{
	int msglen=0;
	uint8 *data=NULL;

	msglen=mqtt_parse_rem_len(pHiP0Data);
	data=&pHiP0Data[3];

	if(msglen>=5 && data[0]==0x00&&data[1]==0x00&&data[2]==0x00)
	{
		xm_soc->handle = data[4];
		switch(data[3])
		{
			case SOCKET_CHECKPASS:
				handlePasscode(xm_soc,&data[5],msglen-5);
				break;
			case SOCKET_TICKET:
				if(msglen>5 && strlen(&data[5]) != (strlen(xcf->snconf.Cloud_SN)+strlen(xcf->snconf.SN_Pass)))
				{
					_xm_SendData2APP(xm_soc,NULL,0,SOCKET_PASS_ERROR);
					break;
				}
				if(memcmp(&data[5],xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN)!=0))
				{
					_xm_SendData2APP(xm_soc,NULL,0,SOCKET_PASS_ERROR);
					break;
				}
				if(memcmp(&data[5+strlen(xcf->snconf.Cloud_SN)],xcf->snconf.SN_Pass,strlen(xcf->snconf.SN_Pass)!=0))
				{
					_xm_SendData2APP(xm_soc,NULL,0,SOCKET_PASS_ERROR);
					break;
				}
				_xm_SendData2APP(xm_soc,NULL,0,CLOUD_SOCKET_TICKETOK);
				break;
			case SOCKET_MSG_PUT:
				if(xcf->callback)
					xcf->callback(xm_soc,&data[5],msglen-5,MSG_CONTROL);
				break;
			case SOCKET_CHANGEPASS:
				change_devpass(xm_soc,&data[5]);
				break;
			default:
				break;
		}
	}
	return ;
}

void Mqtt_DispatchPublishPacket(xm_soc_t *xm_soc,uint8 *packetBuffer,int packetLen )
{
	uint8 topic[170];
	int topiclen;
	uint8 *pHiP0Data=NULL;
	int HiP0DataLen;
	int i;
	uint8  clientid[40];
	uint8 *pTemp;
	uint8  DIDBuffer[32];
	topiclen = mqtt_parse_pub_topic(packetBuffer, topic);
	topic[topiclen] = '\0';

	HiP0DataLen = mqtt_parse_publish_msg(packetBuffer, &pHiP0Data);
	if (pHiP0Data == NULL) 
	{
		return;
	}
	if((strncmp(topic,"app2dev/",strlen("app2dev/"))==0))
	{
		pTemp = &topic[strlen("app2dev/")];
		i = 0;
		while (*pTemp != '/')
		{
			DIDBuffer[i] = *pTemp;
			i++;
			pTemp++;
		}
		DIDBuffer[i] = '\0';

		pTemp ++; 
		i=0;
		while (*pTemp != '\0')
		{
			clientid[i] = *pTemp;
			i++;
			pTemp++;
		}
		clientid[i]= '\0';
		strcpy(xm_soc->clientid,clientid);
		MQTT_DoCloudMCUCmd(xm_soc, pHiP0Data, HiP0DataLen);
	}
}


void init_subTopic(char *Sub_TopicBuf)
{
	memcpy(Sub_TopicBuf,"app2dev/",strlen("app2dev/"));
	memcpy(Sub_TopicBuf+strlen("app2dev/"),xcf->snconf.Cloud_SN,strlen(xcf->snconf.Cloud_SN));

	Sub_TopicBuf[strlen("app2dev/")+strlen(xcf->snconf.Cloud_SN)] = '/';
	Sub_TopicBuf[strlen("app2dev/")+strlen(xcf->snconf.Cloud_SN)+1]='#';
	Sub_TopicBuf[strlen("app2dev/")+strlen(xcf->snconf.Cloud_SN)+2] = '\0';

}

void Mqtt_SubLoginTopic(xm_soc_t *xm_soc)
{
	char Sub_TopicBuf[100]={0};

	g_MQTTStatus = MQTT_STATUS_LOGINTOPIC;
	init_subTopic(Sub_TopicBuf);
	if(mqtt_subscribe(xm_soc,Sub_TopicBuf,&g_Msgsub_id ) == -1)
	{
		;;
	}

	return;
}

void cloud_data_recv(void *param)
{
	uint8 packettype=0;
	int Recmsg_id;
	os_handle_t *xm_os_handle = (os_handle_t*)param;
	uint8 *pdata = xm_os_handle->pdata;
	uint16 len = xm_os_handle->data_len;
	xm_soc_t *xm_soc = xm_os_handle->xm_soc;

	if(len>0 && pdata)
	{
		packettype = MQTTParseMessageType(pdata);

		switch(packettype)
		{
			case MQTT_MSG_PINGRESP:
				xm_debug("pint req\n");
				xm_soc->nowtiming = xm_timestamp();
				break;
			case MQTT_MSG_CONNACK:
				if( g_MQTTStatus == MQTT_STATUS_LOGIN )
				{
					if(pdata[3] != 0x00)
					{
						return;
					}
					Mqtt_SubLoginTopic(xm_soc);
				}
				break;
			case MQTT_MSG_SUBACK:
				Recmsg_id = mqtt_parse_msg_id(pdata);

				if(g_Msgsub_id == Recmsg_id)
				{
					g_MQTTStatus=MQTT_STATUS_RUNNING;
				}
				break;
			case MQTT_MSG_PUBLISH:
				if(g_MQTTStatus==MQTT_STATUS_RUNNING)
				{
					Mqtt_DispatchPublishPacket(xm_soc,pdata,len);
				}
				break ;
			default:
				break;
		}
	}
}
void Connect_cloud()
{
	xm_soc_t *xm_soc=NULL;
	xm_soc = xm_create_socket(CLOUD_TYPE,SOCKET_STREAM_TCP);

	if(xm_soc == NULL){
		xm_debug("xm_soc cloud null\n");
		goto restart;
	}

	if(xm_socket_connect(xm_soc,1883,cloud_server_ip))
	{
		mqtt_connect(xm_soc,xcf->snconf.Cloud_SN);
		return;
	}
restart:
	xm_sendqueue_event(xm_event_call,CLOUD_CONNECT,0);
}
