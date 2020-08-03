/*
MIT License

Copyright (c) 2017 Ulf Lehnert, Helmholtz-Center Dresden-Rossendorf

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/** @mainpage OpcUaServer for the Libera Spark E/L beam position monitors
 *
 *  Version 0.4  2017/02/16
 *
 *  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
 *
 *  This is an OPC UA server running on the
 *  [Libera Spark](http://www.i-tech.si/announcements/performance-reliability-cost-effectiveness--libera-spark)
 *  beam position monitor electronics
 *  for remote control of the devices.
 *  
 *  It is based on the [Open62541](https://github.com/open62541/open62541/)
 *  open source implementation of OPC UA.
 *  
 *  @section Functionality
 *  - Provides an OPC-UA server at TCP/IP port 16664.
 *  - Server configuration is loadad from file /nvram/cfg/opcua.xml
 *  - The /dev/libera.strm0 is captured to obtain the measured data.
 *  - When enabled, all data from strm0 is sent out to an UDP output stream.
 *  - Access to device configuration parameters is handled with the MCI facility.
 *
 *  The server compiles and runs stabily on the power supply used for the tests.
 *  All functionality necessary to user the supllies to power corrector coils
 *  in an accelerator control system environment is provided. This does not
 *  cover the whole functionality provided by the devices, only the essentials.
 *  
 *  @section Build
 *  The server is built with a cross-compiler running on a Linux system
 *  for the ARM target CPU of the device.
 *  
 *  The OPC UA stack needs to be downloaded and built. This can be done on
 *  the development system - there is no binary code produced at this stage.
 *  The complete stack is obtained in two (amalgamated) files.
 *  - open62541.h
 *  - open62541.c
 *  Presently, we use version 1.1 of the OPC UA stack.
 *  
 *  In addition the libxml2 library is required. It needs to be built
 *  and installed into the cross-target tool chain.
 *  
 *  A Makefile is provided, normally just the first line has to be edited to point to
 *  the correct location of the tool chain.
 *
 *  @section Installation
 *  For istallation a few files need to be copied onto the device:
 *  - opcuaserver binary installed in /opt/opcua/opcuaserver
 *  - opcua.xml configuration file in /nvram/cfg/opcua.xml
 *  - libxml2.so.2 in /usr/lib/
 *
 *  The server can then be run by executing /opt/opcua/opcuaserver.
 * 
 *  @section Testing
 *  For a first test of the server an universal OPC UA client like
 *  [UaExpert](https://www.unified-automation.com/products/development-tools/uaexpert.html) is recommended.
 *
 *  A LabView client demonstrating the access using OPC UA is provided in the examples/ folder.
 *
 *  @section TODO
 *  - clean exit problem when UDP stream is never opened
 *
 */

/** @file OpcUaStreamServer.c
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>		     // for flags
#include <sys/stat.h>        // for fstat()
#include <stdlib.h>		     // for exit()
#include <signal.h>		     // for signal()
#include <errno.h>		     // for error messages
#include <pthread.h>         // for threads

#include <netinet/udp.h>	 // declarations for udp header
#include <netinet/ip.h>		 // declarations for ip header
#include <arpa/inet.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "open62541.h"       // the OPC-UA library
#include "libera_mci.h"      // the MCI access routines
#include "libera_opcua.h"    // OPC-UA variable handling

/***********************************/
/* definitions for the data stream */
/***********************************/

#define BLOCKSIZE 64
#define BUFFERSIZE 256		 // must be a multiple of 64

// the data structure sent by the Libera instrument
struct single_pass_data {
   int32_t va;
   int32_t vb;
   int32_t vc;
   int32_t vd;
   int32_t sum;
   int32_t q;
   int32_t x;
   int32_t y;
   uint32_t trigger_cnt;
   uint32_t bunch_cnt;
   uint32_t status;
   uint32_t mode;
   int32_t R2;
   int32_t R3;
   uint64_t time;
};

/***********************************/
/* Server-related variables        */
/***********************************/

// the OPC-UA server
UA_Server *server;

// primary storage of the current values
static int32_t SP_va = 0;
static int32_t SP_vb = 0;
static int32_t SP_vc = 0;
static int32_t SP_vd = 0;
static double SP_charge = 0.0;
static double SP_pos_x = 0.0;
static double SP_pos_y = 0.0;
static double SP_shape_q = 0.0;

// error codes
#define UDP_STREAM_CLOSED -1
#define UDP_STREAM_NO_SOCKET -2
#define UDP_STREAM_GOOD 1

// primary storage of the data streaming information
static int32_t StreamSourceStatus = -1;
static int32_t StreamError = -1;
static bool StreamTransmit = false;
static uint32_t StreamSourceIP = 0;         // 10.66.67.20
static uint32_t StreamSourcePort = 0;
static uint32_t StreamTargetIP = 0;         // 10.66.67.1
static uint32_t StreamTargetPort = 0;

// the OPC-UA variables hosted by this server
/*
    Server
    |   ...
    Device
    |   Name
    |   SampleFreq
    Signals
    |   SP
    |   |   VA
    |   |   VB
    |   |   VC
    |   |   VD
    |   |   Charge
    |   |   PosX
    |   |   PosY
    |   |   ShapeQ
    |   MaxADC
    Stream
    |   StreamStatus
    |   Error
    |   SourceIP
    |   SourcePort
    |   TargetIP
    |   TargetPort
    |   Transmit
    DSP
    |   Enable
    |   BunchThr1
    |   PreTrig
    |   PostTrig1
    |   ScanTimeout
    |   Averaging
    Calibration
    |   AttID
    |   KA
    |   KB
    |   KC
    |   KD
    |   LinearX
    |   LinearY
    |   LinearQ
    |   LinearSum
    |   OffsetX
    |   OffsetY
    |   OffsetQ
    |   OffsetSum
    ClockInfo
*/

// fixed node IDs
#define LIBERA_DEVICE_ID 49000
#define LIBERA_DEVNAME_ID 49100
#define LIBERA_DEVFREQ_ID 49200
#define LIBERA_SIGNALS_ID  50000
#define LIBERA_SP_ID  50100
#define LIBERA_VA_ID  50101
#define LIBERA_VB_ID  50102
#define LIBERA_VC_ID  50103
#define LIBERA_VD_ID  50104
#define LIBERA_CHARGE_ID  50110
#define LIBERA_POSX_ID  50120
#define LIBERA_POSY_ID  50130
#define LIBERA_SHAPEQ_ID  50140
#define LIBERA_MAXADC_ID  50200
#define LIBERA_STREAM_ID 51000
#define LIBERA_STREAMSTATUS_ID 51100
#define LIBERA_STREAMERROR_ID 51110
#define LIBERA_SOURCEIP_ID 51200
#define LIBERA_SOURCEPORT_ID 51210
#define LIBERA_TARGETIP_ID 51300
#define LIBERA_TARGETPORT_ID 51310
#define LIBERA_TRANSMIT_ID 51400
#define LIBERA_DSP_ID 52000
#define LIBERA_DSP_ENABLE_ID 52010
#define LIBERA_DSP_THR1_ID 52020
#define LIBERA_DSP_PRE_ID 52030
#define LIBERA_DSP_POST1_ID 52040
#define LIBERA_DSP_TIMEOUT_ID 52050
#define LIBERA_DSP_AVERAGING_ID 52060
#define LIBERA_CAL_ID 54000
#define LIBERA_CAL_ATT_ID 54010
#define LIBERA_CAL_KA_ID 54110
#define LIBERA_CAL_KB_ID 54120
#define LIBERA_CAL_KC_ID 54130
#define LIBERA_CAL_KD_ID 54140
#define LIBERA_CAL_LINX_ID 54210
#define LIBERA_CAL_LINY_ID 54220
#define LIBERA_CAL_LINQ_ID 54230
#define LIBERA_CAL_LINS_ID 54240
#define LIBERA_CAL_OFFX_ID 54310
#define LIBERA_CAL_OFFY_ID 54320
#define LIBERA_CAL_OFFQ_ID 54330
#define LIBERA_CAL_OFFS_ID 54340

// this variable is a flag for the running server
// when set to false the server stops
UA_Boolean running = true;

/***********************************/
/* interrupt and error handling    */
/***********************************/

// print error message and abort the running program
void Die(char *mess)
{
    fprintf(stderr, "OpcUaServer : %s\n",mess);
    exit(1);
}

// handle SIGINT und SIGTERM
static void stopHandler(int signal)
{
    printf("\nOpcUaServer : received ctrl-c\n");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "received ctrl-c");
    running = 0;
}

/***********************************/
/* stream data handling            */
/***********************************/
/*
    The main programm never acesses any of the streaming data structures.
    All it does is fork off the readStream() thread. That one handles reading
    the data frpm the Libera data stream and update the internal storage
    of the OPC-UA Variables.

    When a client requestes an UDP data stream (by writing StreamTransmit=true)
    the datasource write routine opens the output UDP stream.
    If this goes without errors the readStream() thread now also
    sends out any received data packet via UDP.

    The UDP stream is closed again when a client requests that
    or write errors occur.
*/

// header structure needed for checksum calculation
struct pseudo_header
{
    u_int32_t source_address;
    u_int32_t dest_address;
    u_int8_t placeholder;
    u_int8_t protocol;
    u_int16_t udp_length;
};
 
// data structures for the UDP output stream
static int udp_socket;
uint32_t udp_counter;		           // counter for transmitted UDP packets
static struct sockaddr_in udp_server;
static struct in_addr udp_source;	   // IP address of this BPM
static struct in_addr udp_target;	   // IP address of the server the data is sent to

// generic checksum calculation function
unsigned short csum(unsigned short *ptr, int nbytes)
{
    register long sum;
    unsigned short oddbyte;
    register short answer;
    sum=0;
    while(nbytes>1) {
        sum+=*ptr++;
        nbytes-=2;
    }
    if(nbytes==1) {
        oddbyte=0;
        *((unsigned char*)&oddbyte)=*(unsigned char*)ptr;
        sum+=oddbyte;
    }
    sum = (sum>>16)+(sum & 0xffff);
    sum = sum + (sum>>16);
    answer=(short)~sum;
    return(answer);
}

// open the output stream
int openStreamUDP()
{
    int err = UDP_STREAM_GOOD;
    printf("OpcUaServer : open UDP data stream\n");
    udp_counter = 0;
    // create a raw socket of type IPPROTO
    udp_socket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if(udp_socket == -1)
    {
        printf("OpcUaServer : Failed to create raw socket. Maybe not permitted?\n");
        err = UDP_STREAM_NO_SOCKET;
    };
    // set addresses for the UDP output stream
    udp_source.s_addr = StreamSourceIP;
    udp_target.s_addr = StreamTargetIP;
    // compile the server address
    udp_server.sin_family = AF_INET;
    udp_server.sin_addr.s_addr = udp_target.s_addr;
    udp_server.sin_port = htons(StreamTargetPort);
    printf("OpcUaServer : UDP source IP %s (%d) port %d\n",inet_ntoa(udp_source), udp_source.s_addr, StreamSourcePort);
    printf("OpcUaServer : sending data to IP: %s (%d) port %d\n",inet_ntoa(udp_target), udp_target.s_addr, StreamTargetPort);
    return err;
}

// close the output stream
int closeStreamUDP()
{
    printf("OpcUaServer : close UDP data stream\n");
    int status = close(udp_socket);
    if (status==-1) printf("OpcUaServer : error closing the UDPsocket\n");
    return UDP_STREAM_CLOSED;
}

// special datasource write routine for the Stream/Transmit Variable
// when writing to this datasource the UDP data stream is opened/closed appropriately
static UA_StatusCode
writeTransmit(void *handle, const UA_NodeId nodeid,
              const UA_Variant *data, const UA_NumericRange *range) {
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_BOOLEAN] && data->data)
    {
        bool opcl = *(bool*)data->data;
        *(UA_Boolean*)handle = (UA_Boolean)opcl;
        // *(UA_Boolean*)handle = *(UA_Boolean*)data->data;
        if (opcl == true)
            StreamError = openStreamUDP();
        if (opcl == false)
            StreamError = closeStreamUDP();
    }
    else
    {
        StreamError = -2;
    }
    return UA_STATUSCODE_GOOD;
}

// read the data from the input stream
// when requested copy the data to the output UDP stream
// TODO: if the stream never has any data, the thread blocks
void* readStream(void *arg)
{
    long int counter = 0;
    char readbuffer[BUFFERSIZE];        // buffer for reading from the datat stream
    struct single_pass_data *record;    // pointer to one data block
    
    int fd = *((int *)arg);
    printf("OpcUaServer : reading from fd=%d\n",fd);
    // the record points just to the first block in the read buffer
    record = (struct single_pass_data *) readbuffer;

    struct pseudo_header psh;	        // header for checksum calculation
    char pseudogram[BUFFERSIZE];	    // datagram for checksum calculation
    char udp_buffer[BUFFERSIZE];	    // UDP packet buffer
    // the IP header is at the beginning of the buffer
    struct iphdr *iph = (struct iphdr *) udp_buffer;
    // the UDP header follows after the IP header
    struct udphdr *udph = (struct udphdr *) (udp_buffer + sizeof(struct iphdr));
    // pointer to the payload within the data buffer
    char *databuffer = (char *)(udp_buffer + sizeof(struct iphdr) + sizeof(struct udphdr));
    int PAYLOADSIZE = BUFFERSIZE - sizeof(struct iphdr) - sizeof (struct udphdr);
    if (BLOCKSIZE>PAYLOADSIZE)
        Die("Insufficient buffer size\n");

    while (running)
    {
        int bytes_read = read(fd, readbuffer, BUFFERSIZE);
        // handle read errors
        if (-1 == bytes_read)
        {
            int errsv=errno;
            fprintf(stderr, "OpcUaServer : read() from data stream");
            // usleep(100000);
            sleep(0.1);
        };
        // handle proper data blocks
        if (BLOCKSIZE==bytes_read)
        {
            counter++;
            SP_va = record->va;
            SP_vb = record->vb;
            SP_vc = record->vc;
            SP_vd = record->vd;
            SP_pos_x = 1.e-6 * record->x;
            SP_pos_y = 1.e-6 * record->y;
            SP_charge = 0.0001 * record->sum;
            SP_shape_q = 1.e-6 * record->q;
            // if requested write packet to UDP stream
            if (StreamTransmit && (StreamError == UDP_STREAM_GOOD))
            {
                udp_counter++;
                // clear the packet buffer
                memset(udp_buffer, 0, BUFFERSIZE);
                // fill in the data
                memcpy(databuffer, readbuffer, BLOCKSIZE);
                // fill in the IP Header
                iph->ihl = 5;
                iph->version = 4;
                iph->tos = 0;
                iph->tot_len = sizeof (struct iphdr) + sizeof (struct udphdr) + BLOCKSIZE;
                iph->id = udp_counter;               // Id of this packet
                iph->frag_off = 0;
                iph->ttl = 255;
                iph->protocol = IPPROTO_UDP;
                iph->check = 0;			             // set to 0 before calculating checksum
                iph->saddr = udp_source.s_addr;      // spoof the source IP address
                iph->daddr = udp_target.s_addr;      // receiver IP address
                // IP checksum
                iph->check = csum ((unsigned short *) udp_buffer, iph->tot_len);
                // UDP header
                udph->source = htons (StreamSourcePort);
                udph->dest = htons (StreamTargetPort);
                udph->len = htons(8 + BLOCKSIZE);    // tcp header size
                udph->check = 0;                     // leave checksum 0 now, filled later from pseudo header
                // now compute the UDP checksum using the pseudo header
                psh.source_address = udp_source.s_addr;
                psh.dest_address = udp_target.s_addr;
                psh.placeholder = 0;
                psh.protocol = IPPROTO_UDP;
                psh.udp_length = htons(sizeof(struct udphdr) + BLOCKSIZE );
                memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
                memcpy(pseudogram + sizeof(struct pseudo_header) , udph , sizeof(struct udphdr) + BLOCKSIZE);
                int psize = sizeof(struct pseudo_header) + sizeof(struct udphdr) + BLOCKSIZE;
                udph->check = csum( (unsigned short*) pseudogram , psize);
                //Send the packet
                if (sendto (udp_socket, udp_buffer, iph->tot_len ,  0, (struct sockaddr *) &udp_server, sizeof (udp_server)) < 0)
                {
                    StreamTransmit = false;
                    fprintf(stderr, "OpcUaServer : error sending UDP data stream\n");
                    StreamError = closeStreamUDP();
                };
            };
        };
    };
    printf("OpcUaServer : read thread exit\n");
    pthread_exit(NULL);
}

/***********************************/
/* main program                    */
/***********************************/

int main(int argc, char *argv[])
{

    int fd;			                   // file descriptor
    int status;			               // status of I/O operations
    struct stat stat_buf;              // file status descriptor

    UA_ObjectAttributes object_attr;   // attributes for folders
    UA_VariableAttributes attr;        // attributes for variable nodes

    // initialize and test the MCI system
    if (mci_init() != 0)
        Die("OpcUaServer : Failed to initalize MCI system\n");

    // initialize the XML library and check potential ABI mismatches
    LIBXML_TEST_VERSION
    char buf[80];                      // buffer for reading strings
    int buflen;                        // number of valid characters in the buffer
    UA_String BufString;               // an UA_String representation of the buffer
    xmlDocPtr doc;                     // the resulting document tree
    doc = xmlReadFile("/nvram/cfg/opcua.xml", NULL, 0);
    if (doc == NULL)
        Die("OpcUaServer : Failed to parse XML config file\n");
    // get the root element node
    xmlNode *rootNode = xmlDocGetRootElement(doc);
    xmlNode *configurationNode = NULL;
    for (xmlNode *currNode = rootNode; currNode; currNode = currNode->next)
        if (currNode->type == XML_ELEMENT_NODE)
            if (! strcmp(currNode->name, "configuration"))
                configurationNode = currNode;
    if (configurationNode == NULL)
        Die("OpcUaServer : Failed to find XML <configuration> root node\n");
    xmlNode *opcuaNode = NULL;
    for (xmlNode *currNode = configurationNode->children; currNode; currNode = currNode->next)
        if (currNode->type == XML_ELEMENT_NODE)
            if (! strcmp(currNode->name, "opcua"))
                opcuaNode = currNode;
    if (opcuaNode == NULL)
        Die("OpcUaServer : Failed to find XML <opcua> node\n");
    xmlNode *deviceNode = NULL;
    for (xmlNode *currNode = opcuaNode->children; currNode; currNode = currNode->next)
        if (currNode->type == XML_ELEMENT_NODE)
            if (! strcmp(currNode->name, "device"))
                deviceNode = currNode;
    if (deviceNode == NULL)
        Die("OpcUaServer : Failed to find XML <opcua/device> node\n");
    xmlChar *devicenameProp = xmlGetProp(deviceNode,"name");
    buflen = xmlStrPrintf(buf, 80, "%s", devicenameProp);
    if (buflen == 0)
        Die("OpcUaServer : Failed to read XML <opcua/device> name property\n");
    buf[buflen] = '\0';         // string termination
    printf("OpcUaServer : DeviceName=%s\n", buf);
    BufString = UA_STRING(buf);
    UA_String *DeviceName = UA_String_new();
    UA_String_copy(&BufString, DeviceName);
    xmlNode *streamNode = NULL;
    for (xmlNode *currNode = configurationNode->children; currNode; currNode = currNode->next)
        if (currNode->type == XML_ELEMENT_NODE)
            if (! strcmp(currNode->name, "stream"))
                streamNode = currNode;
    if (streamNode == NULL)
        Die("OpcUaServer : Failed to find XML <stream> node\n");
    xmlNode *streamsourceNode = NULL;
    for (xmlNode *currNode = streamNode->children; currNode; currNode = currNode->next)
        if (currNode->type == XML_ELEMENT_NODE)
            if (! strcmp(currNode->name, "source"))
                streamsourceNode = currNode;
    if (streamsourceNode == NULL)
        Die("OpcUaServer : Failed to find XML <stream/sorce> node\n");
    xmlChar *sourceipProp = xmlGetProp(streamsourceNode,"ip");
    buflen = xmlStrPrintf(buf, 80, "%s", sourceipProp);
    if (buflen == 0)
        Die("OpcUaServer : Failed to read XML <stream/source> ip property\n");
    buf[buflen] = '\0';         // string termination
    printf("OpcUaServer : StreamSourceIP=%s\n", buf);
    BufString = UA_STRING(buf);
    UA_String *StreamSourceIPString = UA_String_new();
    UA_String_copy(&BufString, StreamSourceIPString);
    StreamSourceIP = inet_addr(buf);
    if (StreamSourceIP == INADDR_NONE)
        Die("OpcUaServer : Failed to read XML <stream/source> ip property\n");
    xmlChar *sourceportProp = xmlGetProp(streamsourceNode,"port");
    buflen = xmlStrPrintf(buf, 80, "%s", sourceportProp);
    if (buflen == 0)
        Die("OpcUaServer : Failed to read XML <stream/source> port property\n");
    buf[buflen] = '\0';         // string termination
    if (sscanf(buf, "%d", &StreamSourcePort) != 1)
        Die("OpcUaServer : Failed to read XML <stream/source> port property\n");
    xmlNode *streamtargetNode = NULL;
    for (xmlNode *currNode = streamNode->children; currNode; currNode = currNode->next)
        if (currNode->type == XML_ELEMENT_NODE)
            if (! strcmp(currNode->name, "target"))
                streamtargetNode = currNode;
    if (streamtargetNode == NULL)
        Die("OpcUaServer : Failed to find XML <stream/target> node\n");
    xmlChar *targetipProp = xmlGetProp(streamtargetNode,"ip");
    buflen = xmlStrPrintf(buf, 80, "%s", targetipProp);
    if (buflen == 0)
        Die("OpcUaServer : Failed to read XML <stream/target> ip property\n");
    buf[buflen] = '\0';         // string termination
    printf("OpcUaServer : StreamTargetIP=%s\n", buf);
    BufString = UA_STRING(buf);
    UA_String *StreamTargetIPString = UA_String_new();
    UA_String_copy(&BufString, StreamTargetIPString);
    StreamTargetIP = inet_addr(buf);
    if (StreamTargetIP == INADDR_NONE)
        Die("OpcUaServer : Failed to read XML <stream/target> ip property\n");
    xmlChar *targetportProp = xmlGetProp(streamtargetNode,"port");
    buflen = xmlStrPrintf(buf, 80, "%s", targetportProp);
    if (buflen == 0)
        Die("OpcUaServer : Failed to read XML <stream/target> port property\n");
    buf[buflen] = '\0';         // string termination
    if (sscanf(buf, "%d", &StreamTargetPort) != 1)
        Die("OpcUaServer : Failed to read XML <stream/target> port property\n");
    // done with the XML document
    xmlFreeDoc(doc);
    xmlCleanupParser();

    // server will be running until we receive a SIGINT or SIGTERM
    signal(SIGINT,  stopHandler);
    signal(SIGTERM, stopHandler);

    // configure the UA server
    UA_ServerConfig config;
    memset(&config, 0, sizeof(UA_ServerConfig));
    UA_StatusCode res = UA_ServerConfig_setMinimal(&config, 16664, NULL);
    if(res != UA_STATUSCODE_GOOD)
    {
        printf("UA_ServerConfig_setMinimal() error %8x\n", res);
        exit(-1);
    }
    UA_Server *server = UA_Server_newWithConfig(&config);
    if(!server)
    {
        printf("UA_Server_newWithConfig() failed\n");
        exit(-1);
    }

    /**************************
    Device
    |   Name
    |   SampleFreq
    **************************/

    object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Device");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Device");
    UA_Server_addObjectNode(server,                                        // UA_Server *server
                            UA_NODEID_NUMERIC(1, LIBERA_DEVICE_ID),        // UA_NodeId requestedNewNodeId
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),  // UA_NodeId parentNodeId
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),      // UA_NodeId referenceTypeId
                            UA_QUALIFIEDNAME(1, "Device"),                 // UA_QualifiedName browseName
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),     // UA_NodeId typeDefinition
                            object_attr,                                   // UA_ObjectAttributes attr
                            NULL,                                          // UA_InstantiationCallback
                            NULL);                                         // UA_NodeId *outNewNodeId

    // create the DeviceName variable
    // read-only value defined in the configuration file
    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","device name");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DeviceName");
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalarCopy(&attr.value, DeviceName, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(server,                                       // UA_Server *server
                              UA_NODEID_NUMERIC(1, LIBERA_DEVNAME_ID),      // UA_NodeId requestedNewNodeId
                              UA_NODEID_NUMERIC(1, LIBERA_DEVICE_ID),       // UA_NodeId parentNodeId
                              UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),     // UA_NodeId referenceTypeId
                              UA_QUALIFIEDNAME(1, "DeviceName"),            // UA_QualifiedName browseName
                              UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),  // UA_NodeId typeDefinition
                              attr,                                         // UA_VariableAttributes attr
                              NULL,                                         // UA_InstantiationCallback
                              NULL);                                        // UA_NodeId *outNewNodeId

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","ADC sample frequency");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","SampleFreq");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource devFreqDataSource = (UA_DataSource)
        {
            .read = mci_get_dev_freq,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DEVFREQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DEVICE_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "SampleFreq"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            devFreqDataSource,
            NULL, NULL);
    
    /**************************
    Signals
    |   SP
    |   |   VA
    |   |   VB
    |   |   VC
    |   |   VD
    |   |   Charge
    |   |   PosX
    |   |   PosY
    |   |   ShapeQ
    |   MaxADC
    **************************/

    object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Signals");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Signals");
    UA_Server_addObjectNode(server,                                        // UA_Server *server
                            UA_NODEID_NUMERIC(1, LIBERA_SIGNALS_ID),       // UA_NodeId requestedNewNodeId
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),  // UA_NodeId parentNodeId
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),      // UA_NodeId referenceTypeId
                            UA_QUALIFIEDNAME(1, "Signals"),                // UA_QualifiedName browseName
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),     // UA_NodeId typeDefinition
                            object_attr,                                   // UA_ObjectAttributes attr
                            NULL,                                          // UA_InstantiationCallback *instantiationCallback
                            NULL);                                         // UA_NodeId *outNewNodeId

    object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en_US","SP");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","SP");
    UA_Server_addObjectNode(server,
                            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
                            UA_NODEID_NUMERIC(1, LIBERA_SIGNALS_ID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "SP"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            object_attr,
                            NULL,
                            NULL);

    /*
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_va, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel A raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VA");
    UA_DataSource vaDataSource = (UA_DataSource)
        {
            .handle = &SP_va,           // pointer to the datasource (storage)
            .read = readInt32,          // read method
            .write = writeInt32         // write method
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VA_ID),                 // UA_NodeId requestedNewNodeId
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),                 // UA_NodeId parentNodeId
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),           // UA_NodeId referenceTypeId
            UA_QUALIFIEDNAME(1, "VA"),                          // UA_QualifiedName browseName
            UA_NODEID_NULL,                                     // UA_NodeId typeDefinition
            attr,                                               // UA_VariableAttributes attr
            vaDataSource,                                       // UA_DataSource dataSource
            NULL);                                              // UA_NodeId *outNewNodeId

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_vb, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel B raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VB");
    UA_DataSource vbDataSource = (UA_DataSource)
        {
            .handle = &SP_vb,
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VB_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VB"),
            UA_NODEID_NULL,
            attr,
            vbDataSource,
            NULL);

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_vc, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel C raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VC");
    UA_DataSource vcDataSource = (UA_DataSource)
        {
            .handle = &SP_vc,
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VC_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VC"),
            UA_NODEID_NULL,
            attr,
            vcDataSource,
            NULL);
 
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_vd, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel D raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VD");
    UA_DataSource vdDataSource = (UA_DataSource)
        {
            .handle = &SP_vd,
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VD_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VD"),
            UA_NODEID_NULL,
            attr,
            vdDataSource,
            NULL);

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_charge, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","Bunch charge in pC");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Charge");
    UA_DataSource chargeDataSource = (UA_DataSource)
        {
            .handle = &SP_charge,       // pointer to the datasource (storage)
            .read = readDouble,         // read method
            .write = writeDouble        // write method
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CHARGE_ID),             // UA_NodeId requestedNewNodeId
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),                 // UA_NodeId parentNodeId
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),           // UA_NodeId referenceTypeId
            UA_QUALIFIEDNAME(1, "Charge"),                      // UA_QualifiedName browseName
            UA_NODEID_NULL,                                     // UA_NodeId typeDefinition
            attr,                                               // UA_VariableAttributes attr
            chargeDataSource,                                   // UA_DataSource dataSource
            NULL);                                              // UA_NodeId *outNewNodeId

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_pos_x, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","Position X in mm");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","PosX");
    UA_DataSource posxDataSource = (UA_DataSource)
        {
            .handle = &SP_pos_x,
            .read = readDouble,
            .write = writeDouble
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_POSX_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "PosX"),
            UA_NODEID_NULL,
            attr,
            posxDataSource,
            NULL);

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_pos_y, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","Position Y in mm");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","PosY");
    UA_DataSource posyDataSource = (UA_DataSource)
        {
            .handle = &SP_pos_y,
            .read = readDouble,
            .write = writeDouble
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_POSY_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "PosY"),
            UA_NODEID_NULL,
            attr,
            posyDataSource,
            NULL);

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &SP_shape_q, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","shape parameter q");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","ShapeQ");
    UA_DataSource shapeqDataSource = (UA_DataSource)
        {
            .handle = &SP_shape_q,
            .read = readDouble,
            .write = writeDouble
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_SHAPEQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "ShapeQ"),
            UA_NODEID_NULL,
            attr,
            shapeqDataSource,
            NULL);

    unsigned int varMaxADC;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varMaxADC, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","maximum ADC value");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","MaxADC");
    UA_DataSource maxADCDataSource = (UA_DataSource)
        {
            .handle = &varMaxADC,
            .read = mci_get_maxadc,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_MAXADC_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SIGNALS_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "MaxADC"),
            UA_NODEID_NULL,
            attr,
            maxADCDataSource,
            NULL);
    */

    /**************************
    Stream
    |   StreamStatus
    |   TransmissionStatus
    |   SourceIP
    |   SourcePort
    |   TargetIP
    |   TargetPort
    |   Transmit
    **************************/

    object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en_US","UDP data stream");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Stream");
    UA_Server_addObjectNode(server,
                            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Stream"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            object_attr,
                            NULL,
                            NULL);

    /*
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &StreamSourceStatus, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","Status of the /dev/libera.strm0 source stream");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","StreamStatus");
    UA_DataSource streamstatusDataSource = (UA_DataSource)
        {
            .handle = &StreamSourceStatus,
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_STREAMSTATUS_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "StreamStatus"),
            UA_NODEID_NULL,
            attr,
            streamstatusDataSource,
            NULL);

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &StreamError, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","Status of the output UDP data stream");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Error");
    UA_DataSource streamerrorDataSource = (UA_DataSource)
        {
            .handle = &StreamError,
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_STREAMERROR_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Error"),
            UA_NODEID_NULL,
            attr,
            streamerrorDataSource,
            NULL);
    */

    // create the StreamSourceIP variable
    // read-only value defined in the configuration file
    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","IP number of the data stream sender (ourselves)");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","SourceIP");
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalarCopy(&attr.value, StreamSourceIPString, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_SOURCEIP_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "SourceIP"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            NULL,
            NULL);

    attr = UA_VariableAttributes_default;
    // UA_Variant_setScalar(&attr.value, &StreamSourcePort, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","UDP port number of the data stream sender (ourselves)");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","SourcePort");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource sourceportDataSource = (UA_DataSource)
        {
            // .handle = &StreamSourcePort,
            .read = readUInt32,
            .write = writeUInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_SOURCEPORT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "SourcePort"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            sourceportDataSource,
            &StreamSourcePort, NULL);

    // create the StreamTargetIP variable
    // read-only value defined in the configuration file
    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","IP number of the data stream receiver");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","TargetIP");
    attr.dataType = UA_TYPES[UA_TYPES_STRING].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_Variant_setScalarCopy(&attr.value, StreamTargetIPString, &UA_TYPES[UA_TYPES_STRING]);
    UA_Server_addVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_TARGETIP_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "TargetIP"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            NULL,
            NULL);

    /*
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &StreamTargetPort, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","UDP port number of the data stream receiver");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","TargetPort");
    UA_DataSource targetportDataSource = (UA_DataSource)
        {
            .handle = &StreamTargetPort,
            .read = readUInt32,
            .write = writeUInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_TARGETPORT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "TargetPort"),
            UA_NODEID_NULL,
            attr,
            targetportDataSource,
            NULL);

    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &StreamTransmit, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.description = UA_LOCALIZEDTEXT("en_US","data stream open");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Transmit");
    UA_DataSource transmitDataSource = (UA_DataSource)
        {
            .handle = &StreamTransmit,
            .read = readBool,
            // special write method which handles the requested opening/closing of the stream
            .write = writeTransmit
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_TRANSMIT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Transmit"),
            UA_NODEID_NULL,
            attr,
            transmitDataSource,
            NULL);
    */

    /**************************
    DSP
    |   Enable
    |   BunchThr1
    |   PreTrig
    |   PostTrig1
    |   ScanTimeout
    |   Averaging
    **************************/

    object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en_US","DSP");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","DSP");
    UA_Server_addObjectNode(server,
                            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "DSP"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            object_attr,
                            NULL,
                            NULL);

    /*
    bool varDspEnable;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varDspEnable, &UA_TYPES[UA_TYPES_BOOLEAN]);
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP enable");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspEnable");
    UA_DataSource dspEnableDataSource = (UA_DataSource)
        {
            .handle = &varDspEnable,
            .read = mci_get_dsp_enable,
            .write = mci_set_dsp_enable
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ENABLE_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspEnable"),
            UA_NODEID_NULL,
            attr,
            dspEnableDataSource,
            NULL);

    unsigned int varDspThr1;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varDspThr1, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP bunch threshold 1");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspThr1");
    UA_DataSource dspThr1DataSource = (UA_DataSource)
        {
            .handle = &varDspThr1,
            .read = mci_get_dsp_thr1,
            .write = mci_set_dsp_thr1
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_THR1_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspThr1"),
            UA_NODEID_NULL,
            attr,
            dspThr1DataSource,
            NULL);

    unsigned int varDspPre;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varDspPre, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP number of pre-trigger samples");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspPre");
    UA_DataSource dspPreDataSource = (UA_DataSource)
        {
            .handle = &varDspPre,
            .read = mci_get_dsp_pre,
            .write = mci_set_dsp_pre
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_PRE_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspPre"),
            UA_NODEID_NULL,
            attr,
            dspPreDataSource,
            NULL);

    unsigned int varDspPost1;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varDspPost1, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP number of samples for first frame");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspPost1");
    UA_DataSource dspPost1DataSource = (UA_DataSource)
        {
            .handle = &varDspPost1,
            .read = mci_get_dsp_post1,
            .write = mci_set_dsp_post1
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_POST1_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspPost1"),
            UA_NODEID_NULL,
            attr,
            dspPost1DataSource,
            NULL);

    unsigned int varDspTimeout;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varDspTimeout, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP scan timeout");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspTimeout");
    UA_DataSource dspTimeoutDataSource = (UA_DataSource)
        {
            .handle = &varDspTimeout,
            .read = mci_get_dsp_timeout,
            .write = mci_set_dsp_timeout
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_TIMEOUT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspTimeout"),
            UA_NODEID_NULL,
            attr,
            dspTimeoutDataSource,
            NULL);

    unsigned int varDspAveraging;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varDspAveraging, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP averaging");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspAveraging");
    UA_DataSource dspAveragingDataSource = (UA_DataSource)
        {
            .handle = &varDspAveraging,
            .read = mci_get_dsp_averaging,
            .write = mci_set_dsp_averaging
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_AVERAGING_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspAveraging"),
            UA_NODEID_NULL,
            attr,
            dspAveragingDataSource,
            NULL);
    */

    /**************************
    Calibration
    |   AttID
    |   KA
    |   KB
    |   KC
    |   KD
    |   LinearX
    |   LinearY
    |   LinearQ
    |   LinearSum
    |   OffsetX
    |   OffsetY
    |   OffsetQ
    |   OffsetSum
    **************************/

    object_attr = UA_ObjectAttributes_default;
    object_attr.description = UA_LOCALIZEDTEXT("en_US","Calibration");
    object_attr.displayName = UA_LOCALIZEDTEXT("en_US","Calibration");
    UA_Server_addObjectNode(server,
                            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                            UA_QUALIFIEDNAME(1, "Calibration"),
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                            object_attr,
                            NULL,
                            NULL);
    
    /*
    unsigned int varCalAtt;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalAtt, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en_US","attenuator setting");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Attenuation");
    UA_DataSource calAttDataSource = (UA_DataSource)
        {
            .handle = &varCalAtt,
            .read = mci_get_cal_attenuation,
            .write = mci_set_cal_attenuation
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ATT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Attenuation"),
            UA_NODEID_NULL,
            attr,
            calAttDataSource,
            NULL);

    double varCalKA;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalKA, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel A calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KA");
    UA_DataSource calKADataSource = (UA_DataSource)
        {
            .handle = &varCalKA,
            .read = mci_get_cal_ka,
            .write = mci_set_cal_ka
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KA_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KA"),
            UA_NODEID_NULL,
            attr,
            calKADataSource,
            NULL);

    double varCalKB;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalKB, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel B calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KB");
    UA_DataSource calKBDataSource = (UA_DataSource)
        {
            .handle = &varCalKB,
            .read = mci_get_cal_kb,
            .write = mci_set_cal_kb
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KB_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KB"),
            UA_NODEID_NULL,
            attr,
            calKBDataSource,
            NULL);

    double varCalKC;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalKC, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel C calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KC");
    UA_DataSource calKCDataSource = (UA_DataSource)
        {
            .handle = &varCalKC,
            .read = mci_get_cal_kc,
            .write = mci_set_cal_kc
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KC_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KC"),
            UA_NODEID_NULL,
            attr,
            calKCDataSource,
            NULL);

    double varCalKD;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalKD, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","channel D calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KD");
    UA_DataSource calKDDataSource = (UA_DataSource)
        {
            .handle = &varCalKD,
            .read = mci_get_cal_kd,
            .write = mci_set_cal_kd
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KD_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KD"),
            UA_NODEID_NULL,
            attr,
            calKDDataSource,
            NULL);

    double varCalLinX;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalLinX, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","position X calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearX");
    UA_DataSource calLinXDataSource = (UA_DataSource)
        {
            .handle = &varCalLinX,
            .read = mci_get_cal_linx,
            .write = mci_set_cal_linx
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINX_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearX"),
            UA_NODEID_NULL,
            attr,
            calLinXDataSource,
            NULL);

    double varCalLinY;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalLinY, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","position Y calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearY");
    UA_DataSource calLinYDataSource = (UA_DataSource)
        {
            .handle = &varCalLinY,
            .read = mci_get_cal_liny,
            .write = mci_set_cal_liny
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINY_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearY"),
            UA_NODEID_NULL,
            attr,
            calLinYDataSource,
            NULL);

    double varCalLinQ;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalLinQ, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","shape Q calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearQ");
    UA_DataSource calLinQDataSource = (UA_DataSource)
        {
            .handle = &varCalLinQ,
            .read = mci_get_cal_linq,
            .write = mci_set_cal_linq
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearQ"),
            UA_NODEID_NULL,
            attr,
            calLinQDataSource,
            NULL);

    double varCalLinS;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalLinS, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","sum calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearS");
    UA_DataSource calLinSDataSource = (UA_DataSource)
        {
            .handle = &varCalLinS,
            .read = mci_get_cal_lins,
            .write = mci_set_cal_lins
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINS_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearS"),
            UA_NODEID_NULL,
            attr,
            calLinSDataSource,
            NULL);


    double varCalOffX;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalOffX, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","position X offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetX");
    UA_DataSource calOffXDataSource = (UA_DataSource)
        {
            .handle = &varCalOffX,
            .read = mci_get_cal_offx,
            .write = mci_set_cal_offx
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFX_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetX"),
            UA_NODEID_NULL,
            attr,
            calOffXDataSource,
            NULL);

    double varCalOffY;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalOffY, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","position Y offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetY");
    UA_DataSource calOffYDataSource = (UA_DataSource)
        {
            .handle = &varCalOffY,
            .read = mci_get_cal_offy,
            .write = mci_set_cal_offy
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFY_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetY"),
            UA_NODEID_NULL,
            attr,
            calOffYDataSource,
            NULL);

    double varCalOffQ;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalOffQ, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","shape Q offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetQ");
    UA_DataSource calOffQDataSource = (UA_DataSource)
        {
            .handle = &varCalOffQ,
            .read = mci_get_cal_offq,
            .write = mci_set_cal_offq
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetQ"),
            UA_NODEID_NULL,
            attr,
            calOffQDataSource,
            NULL);

    double varCalOffS;
    attr = UA_VariableAttributes_default;
    UA_Variant_setScalar(&attr.value, &varCalOffS, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en_US","sum offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetS");
    UA_DataSource calOffSDataSource = (UA_DataSource)
        {
            .handle = &varCalOffS,
            .read = mci_get_cal_offs,
            .write = mci_set_cal_offs
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFS_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetS"),
            UA_NODEID_NULL,
            attr,
            calOffSDataSource,
            NULL);
    */

    // open the data stream
    fd = open("/dev/libera.strm0", O_RDONLY);
    if (fd == -1)
    {
        Die("OpcUaServer : failed to open /dev/libera.strm0");
    } else {
        printf("OpcUaServer : opened /dev/libera.strm0 with fd=%d\n",fd);
    };

    if (fstat(fd, &stat_buf) < 0)
        Die("OpcUaServer : fstat() failure on /dev/libera.strm0");

    // fork off a thread that reads the stream data
    pthread_t tid;
    // int pthread_create(
    //      pthread_t *thread,
    //      const pthread_attr_t *attr,
    //      void *(*start_routine) (void *),
    //      void *arg);
    if (0 != pthread_create(&tid, NULL, &readStream, (void *)&fd))
        Die("OpcUaServer : failed to create read thread");
    else
        printf("OpcUaServer : read thread created successfully\n");

    // run the server (forever unless stopped with ctrl-C)
    UA_StatusCode retval = UA_Server_run(server, &running);

    if(retval != UA_STATUSCODE_GOOD)
        printf("UA_Server_run() error %8x\n", retval);

    UA_Server_delete(server);
    printf("OpcUaServer : stopped running.\n");

    // wait for the read thread to exit
    pthread_join(tid, NULL);

    status = close(fd);
    if (-1==status) perror("OpcUaServer : close source stream");
    else printf("OpcUaServer : data stream closed.\n");

    if (StreamTransmit) closeStreamUDP();

    mci_shutdown();

    printf("OpcUaServer : graceful exit\n");
    return (int)retval;
}
