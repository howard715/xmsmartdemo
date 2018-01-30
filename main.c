#include "xmplatform.h"
/*	
char add_json[]="{"	\
	"\"category\":\"interaction\","	\
	"\"ruleid\":1,"	\
    "\"ruleact\":\"add\","	\
    "\"pid\":\"zhang\","	\
    "\"rext\":{"	\
        "\"rulename\":\"test\","	\
        "\"timeoffset\":0,"	\
        "\"week\":[127,1,0],"	\
        "\"stime\":[900,900,2400],"	\
        "\"etime\":[2000,1800,2400],"	\
        "\"state\":1"	\
    "},"	\
    "\"cdit\":{"	\
    	"\"cid\":\"720000000\","	\
        "\"ctype\":\"21\","	\
        "\"act\":11"	\
    "},"	\
    "\"result\":["	\
    	"{"	\
			"\"cpid\":\"N45CCF7F8060A5\","	\
			"\"cptype\":\"09\","	\
        	"\"cid\":\"N45CCF7F8060A5\","	\
            "\"ctype\":\"09\","	\
            "\"act\":1,"	\
            "\"rstate\":1"	\
		 "},"	\
		 "{"	\
		 "\"cpid\":\"N45CCF7F8060A5\","	\
		 "\"cptype\":\"09\","	\
		 "\"cid\":\"N45CCF7F8060A5\","	\
		 "\"ctype\":\"09\","	\
		 "\"act\":1,"	\
		 "\"rstate\":1"	\
	  "},"	\
	  "{"	\
	  "\"cpid\":\"N45CCF7F8060A5\","	\
	  "\"cptype\":\"09\","	\
	  "\"cid\":\"N45CCF7F8060A5\","	\
	  "\"ctype\":\"09\","	\
	  "\"act\":1,"	\
	  "\"rstate\":1"	\
   "},"	\
   "{"	\
   "\"cpid\":\"N45CCF7F8060A5\","	\
   "\"cptype\":\"09\","	\
   "\"cid\":\"N45CCF7F8060A5\","	\
   "\"ctype\":\"09\","	\
   "\"act\":1,"	\
   "\"rstate\":1"	\
"},"	\
"{"	\
"\"cpid\":\"N45CCF7F8060A5\","	\
"\"cptype\":\"09\","	\
"\"cid\":\"N45CCF7F8060A5\","	\
"\"ctype\":\"09\","	\
"\"act\":1,"	\
"\"rstate\":1"	\
"},"	\
"{"	\
"\"cpid\":\"N45CCF7F8060A5\","	\
"\"cptype\":\"09\","	\
"\"cid\":\"N45CCF7F8060A5\","	\
"\"ctype\":\"09\","	\
"\"act\":1,"	\
"\"rstate\":1"	\
"},"	\
"{"	\
"\"cpid\":\"N45CCF7F8060A5\","	\
"\"cptype\":\"09\","	\
"\"cid\":\"N45CCF7F8060A5\","	\
"\"ctype\":\"09\","	\
"\"act\":1,"	\
"\"rstate\":1"	\
"},"	\
"{"	\
"\"cpid\":\"N45CCF7F8060A5\","	\
"\"cptype\":\"09\","	\
"\"cid\":\"N45CCF7F8060A5\","	\
"\"ctype\":\"09\","	\
"\"act\":1,"	\
"\"rstate\":1"	\
"},"	\
"{"	\
"\"cpid\":\"N45CCF7F8060A5\","	\
"\"cptype\":\"09\","	\
"\"cid\":\"N45CCF7F8060A5\","	\
"\"ctype\":\"09\","	\
"\"act\":1,"	\
"\"rstate\":1"	\
"},"	\
"{"	\
"\"cpid\":\"N45CCF7F8060A5\","	\
"\"cptype\":\"09\","	\
"\"cid\":\"N45CCF7F8060A5\","	\
"\"ctype\":\"09\","	\
"\"act\":1,"	\
"\"rstate\":1"	\
"}"	\
    "]"	\
"}";
char get_json[]="{"	\
	"\"category\":\"interaction\","	\
    "\"ruleact\":\"get\","	\
    "\"pid\":\"zhang\"}";

char get_rule_json[]="{"	\
	"\"category\":\"interaction\","	\
	"\"ruleid\":0,"	\
    "\"ruleact\":\"exec\","	\
    "\"pid\":\"zhang\"}";
*/
char *g_eth="enp0s3";
void ximoGetMacAddress(char *pMacAddress,char *ifname)
{     
    int sock_mac;
    struct ifreq ifr_mac;

    if(pMacAddress == NULL)
    {
        return ;
    }
    sock_mac = socket( AF_INET, SOCK_STREAM, 0 );
    if( sock_mac == -1)
    {
        perror("create socket falise...mac/n");
        return ; 
    }

    memset(&ifr_mac,0,sizeof(ifr_mac));
    strncpy(ifr_mac.ifr_name, ifname, sizeof(ifr_mac.ifr_name)-1);
    
    if( (ioctl( sock_mac, SIOCGIFHWADDR, &ifr_mac)) < 0)
    {
        printf("mac ioctl error/n");
        return ;
    }

    pMacAddress[0] = ifr_mac.ifr_hwaddr.sa_data[0];
    pMacAddress[1] = ifr_mac.ifr_hwaddr.sa_data[1];
    pMacAddress[2] = ifr_mac.ifr_hwaddr.sa_data[2];
    pMacAddress[3] = ifr_mac.ifr_hwaddr.sa_data[3];
    pMacAddress[4] = ifr_mac.ifr_hwaddr.sa_data[4];
    pMacAddress[5] = ifr_mac.ifr_hwaddr.sa_data[5];

    close( sock_mac );
}

int getIpAddress(char *iface_name, char *ip_addr)
{
    int sockfd = -1;
    struct ifreq ifr;
    struct sockaddr_in *addr = NULL;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("create socket error!\n");
        return -1;
    }
    
    memset(&ifr, 0, sizeof(struct ifreq));
    strcpy(ifr.ifr_name, iface_name);
    //addr->sin_family = AF_INET;

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == 0) {
    	addr = (struct sockaddr_in *)&ifr.ifr_addr;
        strcpy(ip_addr,inet_ntoa(addr->sin_addr));
        close(sockfd);
        return 0;
    }

    close(sockfd);

    return -1;
}
void get_data(xm_soc_t *xm_soc,uint8 *buf,uint16 len,uint8 msg_type)
{
	if(msg_type == MSG_PASS_CHANGE)
	{
		xm_debug("pass change ,please update the local file\n");
	}else if(msg_type == MSG_CONTROL)
	{
		xm_debug("hello-------clientid %s\nbuf is %s\n",xm_soc->clientid[0] == 0?"null":xm_soc->clientid,buf);
	}
}
/*void *test(void *arg)
{
	sleep(10);
	interacton_json_parse(get_json,0,0,NULL);
	interacton_json_parse(get_rule_json,0,0,NULL);
}*/
xm_conf *xmconf = NULL;
int main(int argc,char **argv)
{
	unsigned char macadd[6]={0x00};
	char ipaddr[20]={0},device[32]={0};
//	pthread_t thid;
//	pthread_create(&thid,NULL,test,NULL);

	xmconf = zalloc(sizeof(xm_conf));
	ximoGetMacAddress(macadd,g_eth);
	xm_debug("howard --- MAC: %02X%02X%02X%02X%02X%02X\n",
			macadd[0],macadd[1],macadd[2],macadd[3],macadd[4],macadd[5]);
#if 0
	sprintf(xmconf->mac,"%02X%02X%02X%02X%02X%02X",
			macadd[0],macadd[1],macadd[2],macadd[3],macadd[4],macadd[5]);
	xm_debug("howard --- MAC: %s\n",xmconf->mac);
#endif
	getIpAddress(g_eth,ipaddr);
	sprintf(xmconf->ipadd,"%s",ipaddr);
	//memcpy(xmconf->ipadd,ipaddr,strlen(ipaddr));
	xm_debug("howard --- test  ---- %s\n",xmconf->ipadd);

	xm_debug("howard --- test  ---- 0\n");
	strcpy(xmconf->snconf.Cloud_SN,"XMR1207D850715");
	strcpy(xmconf->snconf.SN_Pass,"654321");
	xm_debug("howard --- test  ---- 2\n");
	xmconf->callback = get_data;

	//interacton_json_parse(add_json,0,0,NULL);
	xm_debug("start init\n");
	
	xm_cloud_init(xmconf);
	while(1)
	{
		sleep(10);
	}
	
	return 0;
}
