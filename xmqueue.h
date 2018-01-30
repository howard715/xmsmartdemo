#ifndef _XMQUEUE_H
#define _XMQUEUE_H

typedef enum ENUM_EVENT_CMD{
	HTTPLOG_EVENT_WARINGLOG,
	HTTPLOG_EVENT_REGISTERDEV,
	CLOUD_CONNECT,
	DNS_FOUND,
	SOCK_MSG_EVENT,
	EVENT_MAX_VALUE
}event_cmd;

typedef struct _os_handle_{
	xm_soc_t *xm_soc;
	uint8 *pdata;
	uint16 data_len;
	void *udpaddr;
}os_handle_t;

typedef void (*xm_event_callback)(uint32,void*);

void xm_sendqueue_event(xm_event_callback _event_callback,event_cmd  _event_what,void *param);
void xm_event_queue_init();

#endif
