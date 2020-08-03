POKY=/home/lehnertu/Libera/tools/poky/1.8

SYSROOT=$(POKY)/sysroots/x86_64-pokysdk-linux
SDKTARGETSYSROOT=$(POKY)/sysroots/cortexa9-vfp-neon-poky-linux-gnueabi
CONFIG_SITE=$(POKY)/site-config-cortexa9-vfp-neon-poky-linux-gnueabi

TOOLPATH=$(SYSROOT)/usr/bin/arm-poky-linux-gnueabi
CFLAGS=-march=armv7-a -mfloat-abi=softfp -mfpu=neon -mtune=cortex-a9 --sysroot=$(SDKTARGETSYSROOT)
CPPFLAGS=-E -march=armv7-a -mfloat-abi=softfp -mfpu=neon -mtune=cortex-a9 --sysroot=$(SDKTARGETSYSROOT)
LDFLAGS=--sysroot=$(SDKTARGETSYSROOT)
CC=$(TOOLPATH)/arm-poky-linux-gnueabi-gcc $(CFLAGS)
CXX=$(TOOLPATH)/arm-poky-linux-gnueabi-g++ $(CFLAGS)
CPP=$(TOOLPATH)/arm-poky-linux-gnueabi-gcc $(CPPFLAGS)
AS=$(TOOLPATH)/arm-poky-linux-gnueabi-as
LD=$(TOOLPATH)/arm-poky-linux-gnueabi-ld $(LDFLAGS)
STRIP=$(TOOLPATH)/arm-poky-linux-gnueabi-strip
RANLIB=$(TOOLPATH)/arm-poky-linux-gnueabi-ranlib
OBJCOPY=$(TOOLPATH)/arm-poky-linux-gnueabi-objcopy
OBJDUMP=$(TOOLPATH)/arm-poky-linux-gnueabi-objdump
AR=$(TOOLPATH)/arm-poky-linux-gnueabi-ar
NM=$(TOOLPATH)/arm-poky-linux-gnueabi-nm

headers=open62541.h \
	libera_mci.h \
	libera_opcua.h

opcuaserver : OpcUaServer.o open62541.o libera_mci.o libera_opcua.o $(headers)
	$(CXX) -o opcuaserver OpcUaStreamServer.o open62541.o libera_mci.o libera_opcua.o -lpthread -lxml2 -L$(SDKTARGETSYSROOT)/opt/libera/lib -lliberamci -lliberaisig -lliberaistd -lliberainet -lomniORB4 -lomniDynamic4 -lomnithread

OpcUaServer.o : OpcUaServer.c $(headers)
	$(CC) -std=c99 -c -I $(SDKTARGETSYSROOT)/usr/include/libxml2/ OpcUaStreamServer.c

open62541.o : open62541.c $(headers)
	$(CC) -std=c99 -c open62541.c

libera_mci.o : libera_mci.c  $(headers)
	$(CXX) -std=gnu++11 -c -I. -L$(SDKTARGETSYSROOT)/opt/libera/lib libera_mci.c

libera_opcua.o : libera_opcua.c $(headers)
	$(CC) -std=c99 -c libera_opcua.c

clean:
	rm -f *.o
	rm -f opcuaserver

