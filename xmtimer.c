#include "xmplatform.h"
#include "xmtimer.h"

typedef struct _xm_timers_s {
    timer_t             timer;
    xm_timer_callback callback;
    void *              param;
} xm_timer_t;
uint64 xm_timestamp()
{
    struct timeval tv;
    int ret = gettimeofday(&tv, NULL);
    if (ret) {
        return 0;
    }
    uint64 time_in_seconds = tv.tv_sec;
    return time_in_seconds;
}

static void xm_timer_callback_wrapper(union sigval param)
{

    // Which timer expired?
    xm_timer_t *timer = (xm_timer_t *)(param.sival_ptr);

    if (timer != NULL && timer->callback != NULL) {
        timer->callback(timer->param);
    }
}

void* xm_timer_acquire(xm_timer_callback callback,void *param)
{
	xm_timer_t *xm_timer=NULL;
	xm_timer = zalloc(sizeof(xm_timer_t));
	if(xm_timer == NULL)
		return NULL;

	xm_timer->callback = callback;
	xm_timer->param = param;

	struct sigevent se;
	se.sigev_notify = SIGEV_THREAD;
	se.sigev_value.sival_ptr = xm_timer;
	se.sigev_notify_function = xm_timer_callback_wrapper;
	se.sigev_notify_attributes = NULL;
	
	timer_create(CLOCK_REALTIME,&se,&(xm_timer->timer));
	return (void*)xm_timer;
}

void xm_timer_start(void *timer_handle,size_t delay,uint8 type)
{
	xm_timer_t *timer = (xm_timer_t *)timer_handle;
	long long freq_nanosecs = delay * 1000000LL;
	struct itimerspec ts;
	ts.it_value.tv_sec = freq_nanosecs / 1000000000;
       	ts.it_value.tv_nsec = freq_nanosecs % 1000000000;

	if(type == XM_TIMER_ONCE){
		ts.it_interval.tv_sec = 0;
    		ts.it_interval.tv_nsec = 0;
	}else{
		ts.it_interval.tv_sec = ts.it_value.tv_sec;
    		ts.it_interval.tv_nsec = ts.it_value.tv_nsec;
	}
        timer_settime(timer->timer, 0, &ts, NULL);
}
void xm_timer_stop(void *timer_handle)
{
	xm_timer_t *timer = (xm_timer_t *)timer_handle;
	timer_delete(timer->timer);
	free(timer);
}
