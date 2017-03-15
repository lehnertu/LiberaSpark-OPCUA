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

Project status
==============
The server compiles and runs stabily on the devices used for the tests.
The main functionality necessary to use the devices
in an accelerator control system environment is provided. This does not
cover the whole functionality provided by the devices, only the essentials.

Build
==============
The server is built with a cross-compiler running on a Linux system
for the ARM target CPU of the device. An example file for the environment settings
ist provided. In most cases it should be sufficient to edit the first line
of that file to set the correct path to the toolchain.

The OPC UA stack needs to be downloaded and built. This can be done on
the development system - there is no binary code produced at this stage.
The complete stack is obtained in two (amalgamated) files.
- open62541.h
- open62541.c

In addition the libxml2 library is required. It needs to be built
and installed into the cross-target tool chain.

A makefile is not yet provided, just a few lines are required to build the server.
- source ./environment
- $CC -std=c99 -c -I $SDKTARGETSYSROOT/usr/include/libxml2/ OpcUaStreamServer.c
- $CXX -std=gnu++11 -c -I. -L$SDKTARGETSYSROOT/opt/libera/lib libera_mci.c
- $CC -std=c99 -c libera_opcua.c
- $CC -std=c99 -c open62541.c
- $CXX -o opcuaserver OpcUaStreamServer.o open62541.o libera_mci.o libera_opcua.o -lpthread -lxml2
       -L$SDKTARGETSYSROOT/opt/libera/lib -lliberamci -lliberaisig -lliberaistd -lliberainet
       -lomniORB4 -lomniDynamic4 -lomnithread

Installation
============
For istallation a few files need to be copied onto the device:
- opcuaserver binary installed in /opt/opcua/opcuaserver
- opcua.xml configuration file in /nvram/cfg/opcua.xml
- libxml2.so.2 in /usr/lib/

The server can then be run by executing /opt/opcua/opcuaserver.

Testing
=======
For a first test of the server an universal OPC UA client like
[UaExpert](https://www.unified-automation.com/products/development-tools/uaexpert.html) is recommended.

TODO
====
- clean exit problem when UDP stream is never opened

