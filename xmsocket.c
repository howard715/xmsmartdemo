#include "xmplatform.h"
#include "xmsocket.h"
#include "server.h"
#include "Socket.h"
#include "xmtimer.h"
#include "xmqueue.h"
#include "mqttlib.h"

xm_soc_t **xm_ctx =NULL;
void *xm_timer = NULL;
pthread_mutex_t xm_soc_mutex;
void xm_socket_delete(xm_soc_t *xm_soc,int index)
{
	pthread_mutex_lock(&xm_soc_mutex);
	if(xm_ctx[index] == xm_soc)
	{
		close(xm_soc->fd);
		free(xm_soc);
		xm_ctx[index] = NULL;
	}
	pthread_mutex_unlock(&xm_soc_mutex);
}
xm_soc_t* xm_socket_get_cloud()
{
	int i;
	pthread_mutex_lock(&xm_soc_mutex);
	for(i=0;i<MAX_SOCKET_AVL;i++)
	{
		if(xm_ctx[i] && xm_ctx[i]->soc_type == CLOUD_TYPE)
		{
			// xm_debug("delete the soc %d \n",xm_soc->fd);
			pthread_mutex_unlock(&xm_soc_mutex);
			return xm_ctx[i];
		}
	}
	pthread_mutex_unlock(&xm_soc_mutex);
	return NULL;
}
int xm_socket_send(xm_soc_t* xm_soc,const uint8 *data,uint16 len)
{
	int ret = 0;
	int reamining = len;
	while(reamining>0){
		ret = send(xm_soc->fd, &data[len-reamining], reamining, 0);
		if (ret == -1) {
			xm_debug("send error\n");
			xm_soc->soc_valide = false;
			break;
		} else {
			reamining -= ret;
		}
	}
	return ret;
}
void xm_socket_sendto(xm_soc_t* xm_soc,void *udpaddr,const uint8 *data,uint16 len)
{
	struct sockaddr_in *addr = (struct sockaddr_in *)udpaddr;

	if(len <= 0)
		return;
	sendto(xm_soc->fd, data, len, 0, (struct sockaddr*)addr, sizeof(struct sockaddr_in));
}

bool xm_socket_add(xm_soc_t *xm_soc)
{
	int i;
	pthread_mutex_lock(&xm_soc_mutex);
	for(i=0;i<MAX_SOCKET_AVL;i++)
	{
		if(xm_ctx[i] == NULL)
		{
			xm_soc->soc_valide = true;
			xm_soc->nowtiming = xm_timestamp();
			xm_ctx[i] = xm_soc;
			pthread_mutex_unlock(&xm_soc_mutex);
			xm_debug("socket type:%d---fd:%d add to array %d\n",xm_soc->soc_type,xm_soc->fd,i);
			return true;
		}
	}
	pthread_mutex_unlock(&xm_soc_mutex);
	xm_debug("socket type:%d---fd:%d add to array error\n",xm_soc->soc_type,xm_soc->fd);
	close(xm_soc->fd);
	free(xm_soc);
	return false;
}
xm_soc_t *xm_create_socket(uint8 soc_type,SOCKET_STREAM_TYPE stream_type)
{
	xm_soc_t *xm_soc=NULL;
	int set=1;
	struct timeval timeout={0};
	timeout.tv_sec=5;
	timeout.tv_usec=0;
	xm_soc = (xm_soc_t*)zalloc(sizeof(xm_soc_t));
	if(xm_soc == NULL)
		return NULL;
	xm_soc->fd = socket(AF_INET, stream_type==SOCKET_STREAM_TCP?SOCK_STREAM:SOCK_DGRAM, stream_type==SOCKET_STREAM_TCP?IPPROTO_TCP:IPPROTO_UDP);
	xm_soc->soc_type = soc_type;
	if(stream_type==SOCKET_STREAM_UDP)
		setsockopt(xm_soc->fd, SOL_SOCKET, SO_BROADCAST, &set,sizeof(int));
	setsockopt(xm_soc->fd, SOL_SOCKET,SO_RCVTIMEO, (char*)&timeout,sizeof(timeout));
	setsockopt(xm_soc->fd, SOL_SOCKET,SO_SNDTIMEO, (char*)&timeout,sizeof(timeout));
	if(xm_soc->fd<0){
		free(xm_soc);
		xm_soc = NULL;
	}
	return xm_soc;
}

bool xm_socket_bind(xm_soc_t *xm_soc,int bind_port)
{
	struct sockaddr_in addr;

	int opt=1;
	memset(&addr, 0x0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port=htons(bind_port);
	addr.sin_addr.s_addr=htonl(INADDR_ANY);

	setsockopt(xm_soc->fd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(int));
	if(bind(xm_soc->fd, (struct sockaddr *)&addr, sizeof(addr)) != 0)
	{
		close(xm_soc->fd);
		free(xm_soc);
		return false;
	}
	if(xm_soc->soc_type == UDP_LISTEN_TYPE)
		return xm_socket_add(xm_soc);
	return true;
}

bool xm_socket_listen(xm_soc_t *xm_soc)
{
	if(listen(xm_soc->fd, 0) != 0)
	{
		close(xm_soc->fd);
		free(xm_soc);
		return false;
	}
	return xm_socket_add(xm_soc);
}
bool xm_socket_connect(xm_soc_t *xm_soc,uint16 remote_port,uint32 remote_ip)
{
	struct sockaddr_in addr;

	memset(&addr, 0x0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port=htons(remote_port);
	addr.sin_addr.s_addr=remote_ip;
	connect(xm_soc->fd, (const struct sockaddr *) &addr,sizeof(addr));
	return xm_socket_add(xm_soc);
}
xm_soc_t *xm_socket_accept(int socket_fd)
{
	int newClientfd;
	struct sockaddr addr;
	int addrlen= sizeof(addr);
	xm_soc_t *xm_soc = (xm_soc_t*)zalloc(sizeof(xm_soc_t));
	if(xm_soc == NULL)
		return NULL;
	newClientfd = accept(socket_fd, &addr, &addrlen);
	xm_soc->fd = newClientfd;
	xm_soc->soc_type = CLIENT_TYPE;
	if(xm_socket_add(xm_soc))
		return xm_soc;
	return NULL;

}

void xm_socket_close(int socket_fd)
{
	close(socket_fd);
}
// bool xm_sock_write_valide(xm_soc_t *xm_soc)
// {
//     fd_set readfds, exceptfds;
// 	struct timeval t={0};
//     int ret;
// 	FD_ZERO(&readfds);
// 	FD_SET(xm_soc->fd, &readfds);

// 	ret = select((xm_soc->fd+1), &readfds, NULL, &exceptfds, &t);
//     if (ret > 0)
//     {
//         return true;
//     }
//     xm_debug("sockid is %d----ret %d\n",xm_soc->fd,ret);
//     return false;
// }
bool xm_sock_check_timeout(xm_soc_t *xm_soc)
{
	if((xm_soc->soc_type == CLIENT_TYPE && (xm_timestamp() - xm_soc->nowtiming) > 5)
	|| (xm_soc->soc_type == CLOUD_TYPE && (xm_timestamp() - xm_soc->nowtiming) > 150)
	|| (xm_soc->soc_type == HTTPLOG_TYPE && (xm_timestamp() - xm_soc->nowtiming) > 3))
		return false;

	return true;
}
bool xm_sock_read_valide(xm_soc_t *xm_soc)
{
	fd_set readfds;
	struct timeval t={0};
	int ret;
	FD_ZERO(&readfds);
	FD_SET(xm_soc->fd, &readfds);

	ret = select((xm_soc->fd+1), &readfds, NULL, NULL, &t);
	if (ret > 0 && FD_ISSET(xm_soc->fd,&readfds))
	{
		return true;
	}

	if(ret<0 || xm_sock_check_timeout(xm_soc)==false)
		xm_soc->soc_valide = false;
	return false;
}
void xm_socket_cloud_recv(xm_soc_t *xm_soc)
{
	uint8 tmp_rcv_buf[10]={0},*rcv_cloud_data=NULL;
	int bytes_rcvd;

	uint8_t *pData;
	int messageLen;
	int varLen;
	int packet_length=0,total_rcv_len=0;
	int headindex=0;

	while(headindex<5){
		bytes_rcvd = recv(xm_soc->fd, &tmp_rcv_buf[headindex], 1, 0);
		if(bytes_rcvd <= 0)
			return;

		if(headindex>=1 && (tmp_rcv_buf[headindex] & 0x80) != 0x80)
			break;
		headindex++;
	}
	if(headindex>=5)
		return;

	bytes_rcvd = headindex+1;
	pData = tmp_rcv_buf + 0;
	messageLen = mqtt_parse_rem_len(pData);
	varLen = mqtt_num_rem_len_bytes(pData);
	packet_length = varLen + messageLen + 1;

	if(packet_length>0)
		rcv_cloud_data=zalloc(packet_length+1);

	if(rcv_cloud_data==NULL)
		return;

	if (bytes_rcvd <= packet_length)
	{
		memcpy(rcv_cloud_data,tmp_rcv_buf,bytes_rcvd);
		packet_length -= bytes_rcvd;
		total_rcv_len += bytes_rcvd;
	}
	while(packet_length>0)
	{
		bytes_rcvd = recv(xm_soc->fd, &rcv_cloud_data[total_rcv_len], packet_length, 0);
		if(bytes_rcvd < 0)
		{
			free(rcv_cloud_data);
			return;
		}
		packet_length -= bytes_rcvd;
		total_rcv_len += bytes_rcvd;
	}
	xm_soc->pdata = rcv_cloud_data;
	xm_soc->data_len = total_rcv_len;
}

void xm_socket_lan_recv(xm_soc_t *xm_soc)
{
	int fd = xm_soc->fd;
	int recdatalen=0,totalrcv = 0,remainlen=0;
	uint8 *socket_rcvdata=NULL;
	uint8 socketrcvbuf[3]={0};

	recdatalen = recv(fd, socketrcvbuf, 3, 0);
	if(recdatalen < 0 )
		xm_soc->soc_valide =false;

	if(recdatalen == 0 || socketrcvbuf[0]!=0x00)
		return;

	totalrcv=0;
	remainlen=mqtt_parse_rem_len(socketrcvbuf);

	if(remainlen>0)
	{
		socket_rcvdata=zalloc(remainlen+1);
		if(socket_rcvdata==NULL)
			return;

		while(remainlen>0){
			recdatalen = recv(fd, &socket_rcvdata[totalrcv], remainlen, 0);
			if(recdatalen<0)
			{
				xm_soc->soc_valide = false;
				free(socket_rcvdata);
				return;
			}
			remainlen -= recdatalen;
			totalrcv += recdatalen;
		}
	}
	//	xm_debug("socket recv data success fd %d totallen:%d----remainlen %d\n",fd,totalrcv,remainlen);
	xm_soc->pdata = socket_rcvdata;
	xm_soc->data_len = totalrcv;
}
void httplog_response_recv(xm_soc_t *xm_soc)
{
	int fd = xm_soc->fd;
	int recdatalen=0,totalrcv = 0,remainlen=0;
	uint8 *data_start;
	uint8 *pdata=(char*)zalloc(1024+1);
	if(pdata == NULL)
		goto error;
	recdatalen = recv(fd, pdata, 1024, 0);
	if(recdatalen < 0)
		goto error;
	/*	data_start = (char*)strstr(pdata,"\r\n\r\n");
		if(data_start==NULL)
		goto error;
		data_start += 4;
		if(xm_soc->soc_type == UPGRADE_TYPE)
		{
		totalrcv = parse_content_len(pdata);
		xm_soc->seq = totalrcv;

		xm_soc->pdata = data_start;
		xm_soc->data_len = recdatalen-(data_start-pdata);
		totalrcv -= xm_soc->data_len;
		xm_soc->callback(xm_soc);


		while(totalrcv){
	//memset(pdata,0,1024);
	recdatalen = recv(fd, pdata, 1024, 0);
	if(recdatalen < 0)
	goto error;

	xm_soc->pdata = pdata;
	xm_soc->data_len = recdatalen;
	totalrcv -= recdatalen;
	xm_soc->callback(xm_soc);
	}
	goto error;
	}
	*/
	xm_soc->pdata = pdata;
	xm_soc->data_len = recdatalen;
	return;
error:
	xm_soc->pdata = NULL;
	xm_soc->data_len = 0;
	xm_soc->soc_valide = false;
	if(pdata)
		free(pdata);
}
void xm_socket_udpser_recv(xm_soc_t *xm_soc)
{
	int readnum;
	struct sockaddr_in *addr = NULL;
	int addrlen= sizeof(struct sockaddr_in);
	int fd = xm_soc->fd;

	uint8 *buffer=NULL;

	addr = (struct sockaddr_in *)zalloc(addrlen);
	if(addr == NULL)
		goto error;
	buffer = (uint8*)zalloc(200);
	if(buffer == NULL)
		goto error;

	readnum = recvfrom(fd, buffer, 200, 0, (struct sockaddr *)addr, &addrlen);

	if (readnum <= 0)
	{
		goto error;
	}
	xm_soc->pdata = buffer;
	xm_soc->data_len = readnum;
	xm_soc->udpaddr =(void*) addr;
	return;
error:
	if(addr)    free(addr);
	if(buffer) free(buffer);
}

void xm_sock_timercallback(void *param)
{
	int i;
	xm_soc_t *xm_soc;

	for(i=0;i<MAX_SOCKET_AVL;i++){
		if(xm_ctx[i] == NULL)
			continue;
		xm_soc = xm_ctx[i];
		if(xm_soc->soc_valide && xm_sock_read_valide(xm_soc))
		{
			if(xm_sock_check_timeout(xm_soc) == false)
			{
				xm_soc->soc_valide = false;
				goto clean;
			}
			switch(xm_soc->soc_type)
			{
				case UDP_LISTEN_TYPE:
					xm_socket_udpser_recv(xm_soc);
					break;
				case CLIENT_TYPE:
					xm_socket_lan_recv(xm_soc);
					break;
				case CLOUD_TYPE:
					xm_socket_cloud_recv(xm_soc);
					break;
				case HTTPLOG_TYPE:
					xm_debug("http log recv\n");
					httplog_response_recv(xm_soc);
					break;
				default:
					break;
			}
			if((xm_soc->pdata && xm_soc->data_len>0) || xm_soc->soc_type == TCP_LISTEN_TYPE)
			{
				os_handle_t *os_handle = (os_handle_t*)zalloc(sizeof(os_handle_t));
				os_handle->xm_soc = xm_soc;
				os_handle->pdata = xm_soc->pdata;
				os_handle->data_len = xm_soc->data_len;
				os_handle->udpaddr = xm_soc->udpaddr;
				xm_soc->pdata = NULL;
				xm_soc->data_len=0;
				xm_soc->udpaddr = NULL;

				xm_sendqueue_event(xm_event_call,SOCK_MSG_EVENT,(void*)os_handle);
			}
		}
clean:
		if(xm_soc->soc_valide == false)
		{
			xm_debug("close socket %d\n",xm_soc->fd);
			if(xm_soc->soc_type == CLOUD_TYPE){
				xm_socket_delete(xm_soc,i);
				xm_sendqueue_event(xm_event_call,CLOUD_CONNECT,0);
			}else
				xm_socket_delete(xm_soc,i);
		}else if(xm_soc->soc_type == CLOUD_TYPE && xm_timestamp() - xm_soc->nowtiming > 50)
		{
			uint8 packet[2];
			packet[0]=(12<<4);
			packet[1]=0x00;
			xm_socket_send(xm_soc,packet,2);
		}
	}
	xm_timer_start(xm_timer,100,XM_TIMER_ONCE);
}
void xm_socket_Lsendpub(uint8 *data ,uint16 len)
{
	int i;
	uint8 head[8] = {0};
	BuildDataHeader(head,DEVICE_MSG_PUBLISH,len,0);
	for(i=0;i<MAX_SOCKET_AVL;i++)
	{
		if(xm_ctx[i] && xm_ctx[i]->soc_type == CLIENT_TYPE && xm_ctx[i]->soc_valide)
		{
			xm_socket_send(xm_ctx[i],head,8);
			xm_socket_send(xm_ctx[i],data,len);
		}
	}
}

void xm_socket_init()
{
	xm_ctx = (xm_soc_t**)zalloc(sizeof(xm_soc_t*)*MAX_SOCKET_AVL);
	if(xm_ctx == NULL)
	{
		xm_debug("create xm ctx error\n");
		return;
	}
	pthread_mutex_init(&xm_soc_mutex,NULL);
	xm_timer = xm_timer_acquire(xm_sock_timercallback,NULL);
	if(xm_timer == NULL)
		return;

	xm_timer_start(xm_timer,100,XM_TIMER_ONCE);

}
