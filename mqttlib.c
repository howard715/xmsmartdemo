#include "mqttlib.h"
#include "Socket.h"
#include "server.h"

uint8_t mqtt_num_rem_len_bytes(const uint8_t* buf) {
	uint8_t num_bytes = 1;
	
	if ((buf[1] & 0x80) == 0x80) {
		num_bytes++;
		if ((buf[2] & 0x80) == 0x80) {
			num_bytes ++;
			if ((buf[3] & 0x80) == 0x80) {
				num_bytes ++;
			}
		}
	}
	return num_bytes;
}

uint16_t mqtt_parse_rem_len(const uint8_t* buf) {
	uint16_t multiplier = 1;
	uint16_t value = 0;
	uint8_t digit;
	
	buf++;	// skip "flags" byte in fixed header

	do {
		digit = *buf;
		value += (digit & 127) * multiplier;
		multiplier *= 128;
		buf++;
	} while ((digit & 128) != 0);

	return value;
}

/*****************************************************
 *****************************************************
                                                   | PUBLISH                                             | QOS != 0       |
 -------------------------------------------------------------------------------------------
 | flag(1B) | remain length(1~2B)  [ | topic len(2B) | topic data(topic_len B) [ | msg id(2B) ] | msg data(xB) ]
--------------------------------------------------------------------------------------------
                                                   |                   remain length = from topic_len to msg_data                        |                                             
 *****************************************************
 *****************************************************/

/*********************************************************
 ����ֵ: msg id
 *********************************************************/
uint8_t mqtt_parse_msg_id(const uint8_t* buf) {
	uint8_t type = MQTTParseMessageType(buf);
	uint8_t qos = MQTTParseMessageQos(buf);
	uint8_t id = 0;
	
	if(type >= MQTT_MSG_PUBLISH && type <= MQTT_MSG_UNSUBACK) {
        uint8_t rlb = mqtt_num_rem_len_bytes(buf);
		if(type == MQTT_MSG_PUBLISH) {
			if(qos != 0) {
				// fixed header length + Topic (UTF encoded)
				// = 1 for "flags" byte + rlb for length bytes + topic size
				uint8_t offset = *(buf+1+rlb)<<8;	// topic UTF MSB
				offset |= *(buf+1+rlb+1);			// topic UTF LSB
				offset += (1+rlb+2);					// fixed header + topic size
				id = *(buf+offset)<<8;				// id MSB
				id |= *(buf+offset+1);				// id LSB
			}
		} else {
			// fixed header length
			// 1 for "flags" byte + rlb for length bytes
			id = *(buf+1+rlb)<<8;	// id MSB
			id |= *(buf+1+rlb+1);	// id LSB
		}
	}
	return id;
}

/*********************************************************
                                                    | PUBLISH                                             | QOS != 0       |
  -------------------------------------------------------------------------------------------
  | flag(1B) | remain length(1~4B)  [ | topic len(2B) | topic data(topic_len B) [ | msg id(2B) ] | msg data(xB) ]
 --------------------------------------------------------------------------------------------
                                                    |                   remain length = from topic_len to msg_data                        |                                             
 ����: ��topic data �ֶ����ݿ�����topic��������
 ����: topic--��������
 ����ֵ: topic len
 *********************************************************/
uint16_t mqtt_parse_pub_topic(const uint8_t* buf, uint8_t* topic) {
	const uint8_t* ptr;
	uint16_t topic_len = mqtt_parse_pub_topic_ptr(buf, &ptr);
	
	if(topic_len != 0 && ptr != NULL) {
		memcpy(topic, ptr, topic_len);
	}
	
	return topic_len;
}

/*********************************************************
                                                    | PUBLISH                                             | QOS != 0       |
  -------------------------------------------------------------------------------------------
  | flag(1B) | remain length(1~4B)  [ | topic len(2B) | topic data(topic_len B) [ | msg id(2B) ] | msg data(xB) ]
 --------------------------------------------------------------------------------------------
                                                    |                   remain length = from topic_len to msg_data                        |                                             
 ����: ��������buf ������ָ��topic data �ֶε�ָ��
 ����: topic_ptr--��������
 ����ֵ: topic len
 *********************************************************/
uint16_t mqtt_parse_pub_topic_ptr(const uint8_t* buf, const uint8_t **topic_ptr) {
	uint16_t len = 0;
	
	if(MQTTParseMessageType(buf) == MQTT_MSG_PUBLISH) {
		// fixed header length = 1 for "flags" byte + rlb for length bytes
		uint8_t rlb = mqtt_num_rem_len_bytes(buf);
		len = *(buf+1+rlb)<<8;	// MSB of topic UTF
		len |= *(buf+1+rlb+1);	// LSB of topic UTF
		// start of topic = add 1 for "flags", rlb for remaining length, 2 for UTF
		*topic_ptr = (buf + (1+rlb+2));
	} else {
		*topic_ptr = NULL;
	}
	return len;
}

/*********************************************************
                                                    | PUBLISH                                             | QOS != 0       |
  -------------------------------------------------------------------------------------------
  | flag(1B) | remain length(1~4B)  [ | topic len(2B) | topic data(topic_len B) [ | msg id(2B) ] | msg data(xB) ]
 --------------------------------------------------------------------------------------------
                                                    |                   remain length = from topic_len to msg_data                        |                                             
 ����: ��msg data �ֶ����ݿ�����msg ��������
 ����: msg--��������
 ����ֵ:  length of msg data
 *********************************************************/
uint16_t mqtt_parse_publish_msg(const uint8_t* buf, uint8_t** msg) {
	const uint8_t* ptr;	
	uint16_t msg_len = mqtt_parse_pub_msg_ptr(buf, &ptr);
	
	if(msg_len != 0 && ptr != NULL) {
		//memcpy(msg, ptr, msg_len);
		*msg=(uint8_t*)ptr;
	}
	
	return msg_len;
}

/*********************************************************
                                                    | PUBLISH                                             | QOS != 0       |
  -------------------------------------------------------------------------------------------
  | flag(1B) | remain length(1~4B)  [ | topic len(2B) | topic data(topic_len B) [ | msg id(2B) ] | msg data(xB) ]
 --------------------------------------------------------------------------------------------
                                                    |                   remain length = from topic_len to msg_data                        |                                             
 ����: ��������buf ������ָ��msg data �ֶε�ָ��
 ����: msg--��������
 ����ֵ: length of msg data
 *********************************************************/
uint16_t mqtt_parse_pub_msg_ptr(const uint8_t* buf, const uint8_t **msg_ptr) {
	uint16_t len = 0;
	
	if(MQTTParseMessageType(buf) == MQTT_MSG_PUBLISH) {
		// message starts at
		// fixed header length + Topic (UTF encoded) + msg id (if QoS>0)
		uint8_t rlb = mqtt_num_rem_len_bytes(buf);
		uint8_t offset = (*(buf+1+rlb))<<8;	// topic UTF MSB
		offset |= *(buf+1+rlb+1);			// topic UTF LSB
		offset += (1+rlb+2);				// fixed header + topic size

		if(MQTTParseMessageQos(buf)) {
			offset += 2;					// add two bytes of msg id
		}

		*msg_ptr = (buf + offset);
				
		// offset is now pointing to start of message
		// length of the message is remaining length - variable header
		// variable header is offset - fixed header
		// fixed header is 1 + rlb
		// so, lom = remlen - (offset - (1+rlb))
      	len = mqtt_parse_rem_len(buf) - (offset-(rlb+1));
	} else {
		*msg_ptr = NULL;
	}
	return len;
}
void mqtt_assert_seq(xm_soc_t *xm_soc)
{
	if(xm_soc->seq > 65530)
	{
		xm_soc->seq = 0;
	}
}
int mqtt_connect(xm_soc_t* broker,char *clientid)
{
	uint8_t flags = 0x00;
	/****************************add by alex********************************************/
	uint8_t *fixed_header = NULL;
	int fixed_headerLen;
	uint8_t *packet =NULL;
	int packetLen;
	uint16_t offset = 0;
	uint16_t clientidlen,usernamelen,passwordlen,payload_len;
	uint8_t fixedHeaderSize;
	uint8_t remainLen;
	uint8_t var_header[12] = {
		0x00,0x06,0x4d,0x51,0x49,0x73,0x64,0x70, // Protocol name: MQIsdp
		0x03, // Protocol version
		0, // Connect flags
		0, 0, // Keep alive
	};
		
	var_header[10] = MQTT_ALIVE>>8;
	var_header[11] = MQTT_ALIVE&0xFF;

	/***********************************************************************************/	
	
	 clientidlen = strlen(clientid);
	 usernamelen = strlen(USER_ACCOUNT);
	 passwordlen = strlen(USER_PASSWD);
	 payload_len = clientidlen + 2;
	// Preparing the flags
	if(usernamelen) {
		payload_len += usernamelen + 2;
		flags |= MQTT_USERNAME_FLAG;
	}
	if(passwordlen) {
		payload_len += passwordlen + 2;
		flags |= MQTT_PASSWORD_FLAG;
	}
	if(CLEAN_SESSION) {
		flags |= MQTT_CLEAN_SESSION;
	}
	var_header[9]= flags;

   	// Fixed header
     fixedHeaderSize = 2;    // Default size = one byte Message Type + one byte Remaining Length
     remainLen = sizeof(var_header)+payload_len;
    if (remainLen > 127) {
        fixedHeaderSize++;          // add an additional byte for Remaining Length
    }
    
    fixed_header = (uint8_t*)malloc( fixedHeaderSize );
    if( fixed_header==NULL )
    {
        free(fixed_header);
        return -1;
    }
    fixed_headerLen = fixedHeaderSize;
    // Message Type
    fixed_header[0] = MQTT_MSG_CONNECT;

    // Remaining Length
    if (remainLen <= 127) {
        fixed_header[1] = remainLen;
    } else {
        // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes
        fixed_header[1] = remainLen % 128;
        fixed_header[1] = fixed_header[1] | 0x80;
        // second byte is number of 128s
        fixed_header[2] = remainLen / 128;
    }
	
	packet = (uint8_t*)zalloc( fixed_headerLen+sizeof(var_header)+payload_len );
    if( packet==NULL )
    {
        free(fixed_header);
        free(packet);
        return -2;
    }
	packetLen = fixed_headerLen+sizeof(var_header)+payload_len ;

	memcpy(packet, fixed_header, fixed_headerLen);
	free(fixed_header);
	offset += fixed_headerLen;
	memcpy(packet+offset, var_header, sizeof(var_header));
	offset += sizeof(var_header);
	// Client ID - UTF encoded
	packet[offset++] = clientidlen>>8;
	packet[offset++] = clientidlen&0xFF;
	memcpy(packet+offset, clientid, clientidlen);
	offset += clientidlen;

	

	if(usernamelen) {
		// Username - UTF encoded
		packet[offset++] = usernamelen>>8;
		packet[offset++] = usernamelen&0xFF;
		memcpy(packet+offset, USER_ACCOUNT, usernamelen);
		offset += usernamelen;
	}

	if(passwordlen) {
		// Password - UTF encoded
		packet[offset++] = passwordlen>>8;
		packet[offset++] = passwordlen&0xFF;
		memcpy(packet+offset, USER_PASSWD, passwordlen);
		offset += passwordlen;
	}

	xm_socket_send(broker,packet,packetLen);
	
    free(packet);
    return 0;
}


int mqtt_disconnect(xm_soc_t* broker) {
	uint8_t packet[] = {
		MQTT_MSG_DISCONNECT, // Message Type, DUP flag, QoS level, Retain
		0x00 // Remaining length
	};
	xm_socket_send(broker,packet,2);
	return 1;
}

// int ICACHE_FLASH_ATTR mqtt_ping(xm_soc_t* broker) {
// 	// uint8_t packet[] = {
// 	// 	MQTT_MSG_PINGREQ, // Message Type, DUP flag, QoS level, Retain
// 	// 	0x00 // Remaining length
// 	// };

// 	uint8 *packet = (uint8*)zalloc(2*sizeof(uint8));
// 	if(packet == NULL)
// 		return -1;
// 	packet[0]=MQTT_MSG_PINGREQ;
// 	packet[1]=0x00;
// 	xm_socket_send(broker->espconn,packet,2);
// 	free(packet);

// 	return 1;
// }

int XPGmqtt_publish(xm_soc_t* broker, const char* topic, const char* msg, int msgLen, uint8_t retain,uint8_t* head_msg,int head_len) {
	return XPGmqtt_publish_with_qos(broker, topic, msg, msgLen,retain, 0, NULL,head_msg,head_len);
}

int XPGmqtt_publish_with_qos(xm_soc_t* broker,
                             const char* topic, 
                             const char* msg,
                             int msgLen, 
                             uint8_t retain, 
                             uint8_t qos, 
                             uint16_t* message_id,
							 uint8_t* head_msg,int head_len) 
{
	uint16_t topiclen = strlen(topic);

	uint8_t qos_flag = MQTT_QOS0_FLAG;
	uint8_t qos_size = 0; // No QoS included
    uint8_t *var_header=NULL;
    uint8_t *fixed_header=NULL;
    uint8_t	*packet=NULL;
    int var_headerLen;
    int fixed_headerLen;
    int packetLen;
    uint8_t fixedHeaderSize = 2;
    int remainLen;	
	mqtt_assert_seq(broker);	
	if(qos == 1) {
		qos_size = 2; // 2 bytes for QoS
		qos_flag = MQTT_QOS1_FLAG;
	}
	else if(qos == 2) {
		qos_size = 2; // 2 bytes for QoS
		qos_flag = MQTT_QOS2_FLAG;
	}

    /************************add by alex*******************************/
	// Variable header	
	var_header = ( uint8_t* )malloc( topiclen+2+qos_size );
    if( var_header==NULL )
    {
        free(var_header);
        return -1;
    }
	var_headerLen = topiclen+2+qos_size;
    /******************************************************************/
	memset(var_header, 0, var_headerLen);
	var_header[0] = topiclen>>8;
	var_header[1] = topiclen&0xFF;
	memcpy(var_header+2, topic, topiclen);
	if(qos_size) {
		var_header[topiclen+2] = broker->seq>>8;
		var_header[topiclen+3] = broker->seq&0xFF;
		if(message_id) { // Returning message id
			*message_id = broker->seq;
		}
		broker->seq++;
	}

	// Fixed header
	// the remaining length is one byte for messages up to 127 bytes, then two bytes after that
	// actually, it can be up to 4 bytes but I'm making the assumption the embedded device will only
	// need up to two bytes of length (handles up to 16,383 (almost 16k) sized message)
	
	/**********************add by alex**************************************/
	
	remainLen = var_headerLen+head_len+msgLen;	
	/***********************************************************************/
	if (remainLen > 127) {
		fixedHeaderSize++;          // add an additional byte for Remaining Length
	}
	
	/***********************add by alex *******************/	
	fixed_header = (uint8_t*)malloc(fixedHeaderSize );
    if( fixed_header==NULL )
    {
        free(var_header);
        free(fixed_header);
        return -1;
    }
	fixed_headerLen = fixedHeaderSize;
	/******************************************************/
    
   // Message Type, DUP flag, QoS level, Retain
   fixed_header[0] = MQTT_MSG_PUBLISH | qos_flag;
	if(retain) {
		fixed_header[0] |= MQTT_RETAIN_FLAG;
   }
   // Remaining Length
   if (remainLen <= 127) {
       fixed_header[1] = remainLen;
   } else {
       // first byte is remainder (mod) of 128, then set the MSB to indicate more bytes
       fixed_header[1] = remainLen % 128;
       fixed_header[1] = fixed_header[1] | 0x80;
       // second byte is number of 128s
       fixed_header[2] = remainLen / 128;
   }
	/**********************add by alex******************************/
    packetLen = fixed_headerLen+var_headerLen+head_len;
    packet = ( uint8_t* )zalloc(packetLen);
    if(packet==NULL)
    {
        free(var_header);
        free(fixed_header);
        free(packet);
        return -1;
    }

	memcpy(packet, fixed_header, fixed_headerLen);
	memcpy(packet+fixed_headerLen, var_header, var_headerLen);
	if(head_msg != NULL)
		memcpy(packet+fixed_headerLen+var_headerLen,head_msg,head_len);
	
	free(var_header);
    free(fixed_header);

	xm_socket_send(broker,packet,packetLen);
	free(packet);
	if(msg != NULL)
		xm_socket_send(broker,msg,msgLen);
	return 1;
}

int mqtt_pubrel(xm_soc_t* broker, uint16_t message_id) {
	uint8_t packet[4] = {
		MQTT_MSG_PUBREL | MQTT_QOS1_FLAG, // Message Type, DUP flag, QoS level, Retain
		0x02, // Remaining length
		0/*message_id>>8*/,
		0/*message_id&0xFF*/
	};
	/****************************************/
	xm_socket_send(broker,packet,4);

	return 1;
}

int mqtt_subscribe(xm_soc_t* broker, const char* topic, uint16_t* message_id) {
	uint16_t topiclen = strlen(topic);
	
	/******************add by alex*********************/
	uint8_t *utf_topic=NULL;
	uint8_t *packet=NULL;	
	int utf_topicLen;
	int packetLen;	
	uint8_t fixed_header[2];
	mqtt_assert_seq(broker);
	/*************************************************/
	
	// Variable header
	uint8_t var_header[2]; // Message ID
	var_header[0] = broker->seq>>8;
	var_header[1] = broker->seq&0xFF;
	if(message_id) { // Returning message id
		*message_id = broker->seq;
	}
	broker->seq++;

	/*******************add by alex**************************/
	// utf topic
	//uint8_t utf_topic[topiclen+3]; // Topic size (2 bytes), utf-encoded topic, QoS byte
	utf_topic = ( uint8_t*)malloc( topiclen+3 );
    if( utf_topic==NULL )
    {
        free(utf_topic);
        return -1;
    }
	utf_topicLen = topiclen+3;
	memset(utf_topic, 0, utf_topicLen);
	utf_topic[0] = topiclen>>8;
	utf_topic[1] = topiclen&0xFF;
	memcpy(utf_topic+2, topic, topiclen);
	/*********************************************************/
	
	/********************add by alex***********************/
	// Fixed header
	fixed_header[0] = MQTT_MSG_SUBSCRIBE | MQTT_QOS1_FLAG;
	fixed_header[1] = sizeof(var_header)+utf_topicLen;
	/******************************************************/
	
	/***********************add by alex********************/		
	packetLen = sizeof(var_header)+sizeof(fixed_header)+utf_topicLen;
	packet = (uint8_t*)malloc( packetLen );
	if( packet==NULL )
    {
        free(utf_topic);
        free(packet);
        return -1;
    }
	memset(packet, 0, packetLen);
	memcpy(packet, fixed_header, sizeof(fixed_header));
	memcpy(packet+sizeof(fixed_header), var_header, sizeof(var_header));
	memcpy(packet+sizeof(fixed_header)+sizeof(var_header), utf_topic, utf_topicLen);

	xm_socket_send(broker,packet,packetLen);
	free(packet);
    free(utf_topic);
    //free(packet);
	return 1;
}

int mqtt_unsubscribe(xm_soc_t* broker, const char* topic, uint16_t* message_id) {
	
	uint16_t topiclen = strlen(topic);

	/******************add by alex*******************/
	uint8_t *utf_topic=NULL;
	int utf_topicLen;
	uint8_t fixed_header[2];
	uint8_t *packet = NULL;
	int packetLen;
	mqtt_assert_seq(broker);
	/************************************************/
	// Variable header
	uint8_t var_header[2]; // Message ID
	var_header[0] = broker->seq>>8;
	var_header[1] = broker->seq&0xFF;
	if(message_id) { // Returning message id
		*message_id = broker->seq;
	}
	broker->seq++;
	/******************add by alex**********************************/
	// utf topic	
	utf_topic = (uint8_t*)malloc( topiclen+2 );
    if( utf_topic==NULL )
    {
        free( utf_topic );
        return -1;
    }
	utf_topicLen = topiclen+2;
	memset(utf_topic, 0, utf_topicLen);
	utf_topic[0] = topiclen>>8;
	utf_topic[1] = topiclen&0xFF;
	memcpy(utf_topic+2, topic, topiclen);
	/***************************************************************/

	/*************************add by alex*******************************/
	// Fixed header
	fixed_header[0] = MQTT_MSG_UNSUBSCRIBE | MQTT_QOS1_FLAG;
	fixed_header[1] = sizeof(var_header)+utf_topicLen;
	
	packetLen = sizeof(var_header)+sizeof(fixed_header)+utf_topicLen;
	packet = (uint8_t*)malloc( packetLen );
    if( packet==NULL )
    {
        free(utf_topic);
        free( packet );
        return -1;
    }
	memset(packet, 0, packetLen);
	memcpy(packet, fixed_header, sizeof(fixed_header));
	memcpy(packet+sizeof(fixed_header), var_header, sizeof(var_header));
	memcpy(packet+sizeof(fixed_header)+sizeof(var_header), utf_topic, utf_topicLen);

	xm_socket_send(broker,packet,packetLen);
	free(packet);
    free(utf_topic);
	return 1;
}


