#include "xmplatform.h"
#include "xmqueue.h"

typedef struct _os_event_ {
	xm_event_callback callback;
	event_cmd event;
	void* param;
	struct _os_event_ *pre;
	struct _os_event_ *next;
} os_event_t;

typedef struct _XM_EVENT_{
	pthread_t	task;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	os_event_t *head;
	os_event_t *tail;
}xm_event_t;

xm_event_t *xm_event=NULL;
void xm_sendqueue_event(xm_event_callback _event_callback,event_cmd _event_what,void *param)
{
	os_event_t *e;
	e = (os_event_t*)zalloc(sizeof(os_event_t));
	e->event = _event_what;
	e->callback = _event_callback;
	e->pre = NULL;

	e->param = param;

	pthread_mutex_lock(&xm_event->mutex);
	if(xm_event->head)
		xm_event->head->pre = e;
	e->next = xm_event->head;
	xm_event->head = e;

	if(xm_event->tail == NULL)
		xm_event->tail = e;
	pthread_mutex_unlock(&xm_event->mutex);
	if(xm_event->tail == e)
		pthread_cond_signal(&xm_event->cond);
	
}
/*
uint32 local_func_run(uint32 time)
{
	uint32 nowtime = time;
	if((xm_timestamp() - nowtime)>10){
		http_check_deviceregister();

		//Timerule_check();
		nowtime = xm_timestamp();
	}
	return nowtime;
}*/
void *xQueueRecive_task (void *arg) {
	os_event_t *e;
	uint32 nowtime = 0;
	xm_event_t *events = (xm_event_t*)arg;

	do {
		pthread_mutex_lock(&events->mutex);
		while(events->tail == NULL)
			pthread_cond_wait(&events->cond,&events->mutex);
		e = events->tail;
		events->tail = events->tail->pre;
		if(events->tail == NULL)
			events->head = NULL;
		pthread_mutex_unlock(&events->mutex);

		if(e->callback)
			e->callback(e->event,e->param);

		free(e);
		e=NULL;
		//nowtime = local_func_run(nowtime);
	}while(1);

	return NULL;
}
void xm_event_queue_init()
{
	xm_event = (xm_event_t*)zalloc(sizeof(xm_event_t));
	if( xm_event == NULL )
		return;
	pthread_mutex_init(&xm_event->mutex,NULL);
	pthread_cond_init(&xm_event->cond,NULL);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	pthread_create(&xm_event->task,&attr,xQueueRecive_task,(void*)xm_event);
	pthread_attr_destroy(&attr);
}
