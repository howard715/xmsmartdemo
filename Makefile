exe := democloud
libobjs := cJSON.o httplog.o mqttlib.o mqttxpg.o server.o Socket.o xmqueue.o xmsocket.o xmtimer.o
exeobjs := main.o
syslib = -lpthread -lxml2 -luci -lrt -lm

CFLAGS += -D_GNU_SOURCE
XM_FLAGS=1
ifeq ($(XM_FLAGS),1)
CFLAGS += -I/usr/include/libxml2
else
CFLAGS += -I/home/howard/mtk-sdk-openwrt/staging_dir/target-mipsel_24kec+dsp_uClibc-0.9.33.2/usr/include/libxml2
endif

all: $(exe)

$(exe) : $(exeobjs) $(libobjs)
	$(CC) $(CFLAGS) -o $@ $^ $(syslib)

$(exeobjs): %.o : %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(libobjs): %.o : %.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	-rm -f *.o *.bin $(exe) online *.info *.db *.xml
run:
	./$(exe)
