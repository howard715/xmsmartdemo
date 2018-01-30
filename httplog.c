#include "xmplatform.h"
#include "httplog.h"
#include "xmqueue.h"
#include "server.h"
#include "xmsocket.h"
#include "xmtimer.h"

bool isregister = false;
void *regdev_timer=NULL;

void http_check_deviceregister() {

	if(isregister)
	{
		xm_timer_stop(regdev_timer);
		regdev_timer = NULL;
		return;
	}
	char *send_data=NULL;

	send_data = (char*)zalloc(200);
	if(send_data == NULL)
		goto restart;

	sprintf(send_data,"{\"sn\":\"%s\",\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"pwd\":\"%s\",\"desc\":\"\"}",xcf->snconf.Cloud_SN,
			xcf->mac[0],xcf->mac[1],xcf->mac[2],xcf->mac[3],xcf->mac[4],xcf->mac[5],xcf->snconf.SN_Pass);

	xm_debug("send reg data %s\n",send_data);
	Http_SendDataServer(send_data,1);
restart:
	xm_timer_start(regdev_timer,10000,XM_TIMER_ONCE);
}

void http_register_dev()
{
	regdev_timer = xm_timer_acquire(http_check_deviceregister,NULL);
	if(regdev_timer == NULL)
		return;
	xm_debug("start register dev\n");
	xm_timer_start(regdev_timer,1000,XM_TIMER_ONCE);
}

void http_Receive(void *param)
{
	if(isregister)
		return;

	os_handle_t *xm_os_handle = (os_handle_t*)param;
	uint8 *pdata = xm_os_handle->pdata;
	uint16 len = xm_os_handle->data_len;

	xm_debug("httplog data is %s\n",(char*)pdata);
	if(strstr(pdata,"success") || strstr(pdata,"8012"))
	{
		isregister = true;
	}
}
char* init_send_data(char *data,char *uri)
{
	int ContentLen = strlen(data);
	char *postBuf;
	postBuf = (char*)zalloc(300+ContentLen);
	if(postBuf == NULL){
		return NULL;
	}

	sprintf( postBuf,"POST %s HTTP/1.1\r\n"
			"Host: api.ximotech.com\r\n"
			"X-Ximo-Product-Key: %s\r\n"
			"Content-Length: %d\r\n"
			"Content-Type: application/json\r\n\r\n"
			"%s",uri,PRODUCTKEY,ContentLen,data);
	return postBuf;
}
void Http_SendDataServer(char *data,char type) {
	char *senddata=NULL;
	xm_soc_t *xm_soc =NULL;
	if(type==0){
		if(isregister)
			senddata = (void*)init_send_data(data,"/api/device/devicelog");
		else{
			xm_debug("device is not register \n");
		}
	}
	else{
		senddata = (void*)init_send_data(data,"/api/device/device");
	}
	if(data)
		free(data);

	if(cloud_server_ip == 0 || senddata == NULL)
		goto error;

	xm_soc = xm_create_socket(HTTPLOG_TYPE,SOCKET_STREAM_TCP);

	if(xm_soc == NULL){
		xm_debug("xm_soc httplog null\n");
		goto error;
	}

	if(xm_socket_connect(xm_soc,80,cloud_server_ip))
	{
		xm_socket_send(xm_soc,senddata,strlen(senddata));
	}
error:
	if(senddata)
		free(senddata);
}

