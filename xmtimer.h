#ifndef _XM_TIMER_H_
#define _XM_TIMER_H_

#define XM_TIMER_ONCE 0x01
#define XM_TIMER_MUTI 0x02

typedef void (*xm_timer_callback)(void *);

void* xm_timer_acquire(xm_timer_callback callback,void *param);

void xm_timer_start(void *timer,size_t delay,uint8 type);
void xm_timer_stop(void *timer);

uint64 xm_timestamp();

#endif
