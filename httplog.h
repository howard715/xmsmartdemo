/*
 * http.h
 *
 *  Created on: 2016��7��15��
 *      Author: zzapp
 */

#ifndef _HTTPLOG_H_
#define _HTTPLOG_H_

void Http_SendDataServer(char *data,char type);
void http_Receive(void *param);
void http_register_dev();

#endif /* APP_XMCLOUD_HTTP_H_ */
