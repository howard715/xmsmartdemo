
#ifndef __MQTTLIB_H__
#define __MQTTLIB_H__

#include "xmsocket.h"
#define MQTT_STATUS_LOGIN           (1<<1)
#define MQTT_STATUS_LOGINTOPIC     (1<<3)
#define MQTT_STATUS_RUNNING         (1<<4)

void Connect_cloud();
void xm_SendPublishPacket(uint8 *uartbuf,int uartbufLen);
void xm_SendData2APP(xm_soc_t *xm_soc,uint8 *uartbuf,int uartbufLen);
int _xm_SendData2APP(xm_soc_t* xm_soc,uint8 *uartbuf,int uartbufLen,uint8 cmd);
void xm_SendData2Dev(uint8 *uartbuf,int uartbufLen,uint8 *devsn);
void cloud_data_recv(void *param);
#endif // __LIBEMQTT_H__
