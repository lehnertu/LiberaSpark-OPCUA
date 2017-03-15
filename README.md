# OPC UA server for the Libera Spark BPM electronics

This is an OPC UA server running on the
[Libera Spark](http://www.i-tech.si/accelerators-instrumentation/spark-el-hl/)
beam position processors for remote control of the devices.

It is based on the [Open62541](https://github.com/open62541/open62541/)
open source implementation of OPC UA.

# Functionality
- Provides an OPC-UA server at TCP/IP port 16664.
- Access to device is handled via the internal MCI facility.
- Server configuration is loadad from file /nvram/cfg/opcua.xml
- The /dev/libera.strm0 is captured to obtain the measured data.
- When enabled, all data from strm0 is sent out to an UDP output stream.

# Project status
The server compiles and runs stabily on the devices used for the tests.
The main functionality necessary to use the devices
in an accelerator control system environment is provided. This does not
cover the whole functionality provided by the devices, only the essentials.

# Build
## Tool chain
The server is built with a cross-compiler running on a Linux system
for the ARM target CPU of the device. An example file for the environment settings
ist provided. In most cases it should be sufficient to edit the first line
of that file to set the correct path to the toolchain.

## Protocol stack
The OPC UA stack needs to be downloaded from [Open62541](https://github.com/open62541/open62541/) and built.
This can be done on the development system - there is no binary code produced at this stage.
The complete stack is obtained in two (amalgamated) files.
- open62541.h
- open62541.c

## Libraries
### Compile and install libxml2 into the toolchain
In addition the libxml2 library is required. It needs to be built
and installed into the cross-target tool chain.
- `tar -xvzf libxml2-2.9.2.tar.gz`
- `cd libxml2-2.9.4`
- `source ../environment`
- `./configure --host=arm-linux --without-iconv --without-python --prefix=$SDKTARGETSYSROOT/usr/`
- `make`
- `make install`


### Install boost libraries into the toolchain
Depending on the version of the toolchain some required boost libraries need to be installed.
At present we use the version `libboost1.55-dev_1.55.0+dfsg-3_armel.deb`.
This package was unpacked manually and the necessary files installed from
`.../usr/include/boost/` into `$SDKTARGETSYSROOT/usr/include`

## Libera libraries (for MCI)
To use the MCI facility to access the internal instrument data a few libraries proprietary to
[Instrumentation Technologies]http://www.i-tech.si/. These were obtained in two files
- `libera-base3.0-dev_3.0-426+r23640+helium_armelx.deb`
- `libera-mci3.0-dev_3.0-426+r23640+helium_armelx.deb`

The packages were manually extracted and theh eaders copied into local directories for MCI programming
- `LiberaSpark-OPCUA/istd/*`
- `LiberaSpark-OPCUA/mci/*`
- `LiberaSpark-OPCUA/isig/*`

The binaries can be copied from the instrument
- from `/opt/libera/lib/*`
- to `$SDKTARGETSYSROOT/opt/libera/lib`

In addition, one needs to create symbolic links like `ln -s libliberamci.so.3.0 libliberamci.so`

The libraries obtained this way should include
- `liberamci.so`
- `liberaisig.so`
- `liberaistd.so`
- `liberainet.so`

In the same way another 3 binary libraries need to be installed
- `$SDKTARGETSYSROOT/usr/lib/libomniORB4.so.2.0`
- `$SDKTARGETSYSROOT/usr/lib/libomnithread.so.4.0`
- `$SDKTARGETSYSROOT/usr/lib/libomniDynamic4.so.2.0`

## Compile and link the server
A makefile is not yet provided, just a few lines are required to build the server.
- `source ./environment`
- `$CC -std=c99 -c -I $SDKTARGETSYSROOT/usr/include/libxml2/ OpcUaStreamServer.c`
- `$CXX -std=gnu++11 -c -I. -L$SDKTARGETSYSROOT/opt/libera/lib libera_mci.c`
- `$CC -std=c99 -c libera_opcua.c`
- `$CC -std=c99 -c open62541.c`
- `$CXX -o opcuaserver OpcUaStreamServer.o open62541.o libera_mci.o libera_opcua.o -lpthread -lxml2
       -L$SDKTARGETSYSROOT/opt/libera/lib -lliberamci -lliberaisig -lliberaistd -lliberainet
       -lomniORB4 -lomniDynamic4 -lomnithread`

# Installation
For istallation a few files need to be copied onto the device:
- `opcuaserver` binary installed to `/opt/opcua/opcuaserver`
- `opcua.xml` configuration file in `/nvram/cfg/opcua.xml`
- `/usr/lib/libxml2.so.2`

The file `opcua.xml` needs to be edited. It containes the settings op IP addresses and port numbers for the
UDP data stream and the device name.

The server can then be run by executing /opt/opcua/opcuaserver. It is recommended to call it by
an init script at boot time of the device.

# Testing
For a first test of the server an universal OPC UA client like
[UaExpert](https://www.unified-automation.com/products/development-tools/uaexpert.html) is recommended.

# TODO - known bugs
- clean exit problem when UDP stream is never opened

