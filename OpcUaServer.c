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
 *  Version 1.1  2020/08/03
 *
 *  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
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

// primary storage of the data streaming information
static bool StreamEnable = false;
static int32_t StreamStatus = -1;
static uint32_t StreamSourceIP = 0;         // 10.66.67.21
static uint64_t StreamSourceMAC = 0;		// 01:26:32:00:06:D4
static uint32_t StreamSourcePort = 2048;
static uint32_t StreamTargetIP = 0;         // 10.66.67.1
static uint64_t StreamTargetMAC = 0;		// 00:40:9e:04:15:3a
static uint32_t StreamTargetPort = 2048;

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
    |   Enable
    |   Status
    |   SourceIP
    |   SourceMAC
    |   SourcePort
    |   TargetIP
    |   TargetMAC
    |   TargetPort
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
#define LIBERA_STREAMENABLE_ID 51100
#define LIBERA_STREAMSTATUS_ID 51200
#define LIBERA_SOURCEIP_ID 51310
#define LIBERA_SOURCEMAC_ID 51320
#define LIBERA_SOURCEPORT_ID 51330
#define LIBERA_TARGETIP_ID 51410
#define LIBERA_TARGETMAC_ID 51420
#define LIBERA_TARGETPORT_ID 51430
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
*/

// read the data from the input stream
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

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel A raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VA");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource vaDataSource = (UA_DataSource)
        {
            .read = readInt32,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VA_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VA"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            vaDataSource,
            &SP_va, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel B raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VB");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource vbDataSource = (UA_DataSource)
        {
            .read = readInt32,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VB_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VB"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            vbDataSource,
            &SP_vb, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel C raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VC");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource vcDataSource = (UA_DataSource)
        {
            .read = readInt32,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VC_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VC"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            vcDataSource,
            &SP_vc, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel D raw signal");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","VD");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource vdDataSource = (UA_DataSource)
        {
            .read = readInt32,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_VD_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "VD"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            vdDataSource,
            &SP_vd, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","Bunch charge in pC");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Charge");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource chargeDataSource = (UA_DataSource)
        {
            .read = readDouble,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CHARGE_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Charge"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            chargeDataSource,
            &SP_charge, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","Position X in mm");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","PosX");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource posxDataSource = (UA_DataSource)
        {
            .read = readDouble,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_POSX_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "PosX"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            posxDataSource,
            &SP_pos_x, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","Position Y in mm");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","PosY");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource posyDataSource = (UA_DataSource)
        {
            .read = readDouble,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_POSY_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "PosY"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            posyDataSource,
            &SP_pos_y, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","shape parameter q");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","ShapeQ");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource shapeqDataSource = (UA_DataSource)
        {
            .read = readDouble,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_SHAPEQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "ShapeQ"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            shapeqDataSource,
            &SP_shape_q, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","maximum ADC value");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","MaxADC");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ;
    UA_DataSource maxADCDataSource = (UA_DataSource)
        {
            .read = mci_get_maxadc,
            .write = NULL
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_MAXADC_ID),
            UA_NODEID_NUMERIC(1, LIBERA_SIGNALS_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "MaxADC"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            maxADCDataSource,
            NULL, NULL);

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

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","Status of the /dev/libera.strm0 source stream");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","StreamStatus");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource streamstatusDataSource = (UA_DataSource)
        {
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_STREAMSTATUS_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "StreamStatus"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            streamstatusDataSource,
            &StreamSourceStatus, NULL);


    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","Status of the output UDP data stream");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Error");
    attr.dataType = UA_TYPES[UA_TYPES_INT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource streamerrorDataSource = (UA_DataSource)
        {
            .read = readInt32,
            .write = writeInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_STREAMERROR_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Error"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            streamerrorDataSource,
            &StreamError, NULL);

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
    attr.description = UA_LOCALIZEDTEXT("en_US","UDP port number of the data stream sender (ourselves)");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","SourcePort");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource sourceportDataSource = (UA_DataSource)
        {
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

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","UDP port number of the data stream receiver");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","TargetPort");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource targetportDataSource = (UA_DataSource)
        {
            .read = readUInt32,
            .write = writeUInt32
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_TARGETPORT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_STREAM_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "TargetPort"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            targetportDataSource,
            &StreamTargetPort, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","data stream open");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Transmit");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource transmitDataSource = (UA_DataSource)
        {
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
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            transmitDataSource,
            &StreamTransmit, NULL);

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

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP enable");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspEnable");
    attr.dataType = UA_TYPES[UA_TYPES_BOOLEAN].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dspEnableDataSource = (UA_DataSource)
        {
            .read = mci_get_dsp_enable,
            .write = mci_set_dsp_enable
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ENABLE_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspEnable"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            dspEnableDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP bunch threshold 1");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspThr1");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dspThr1DataSource = (UA_DataSource)
        {
            .read = mci_get_dsp_thr1,
            .write = mci_set_dsp_thr1
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_THR1_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspThr1"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            dspThr1DataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP number of pre-trigger samples");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspPre");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dspPreDataSource = (UA_DataSource)
        {
            .read = mci_get_dsp_pre,
            .write = mci_set_dsp_pre
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_PRE_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspPre"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            dspPreDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP number of samples for first frame");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspPost1");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dspPost1DataSource = (UA_DataSource)
        {
            .read = mci_get_dsp_post1,
            .write = mci_set_dsp_post1
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_POST1_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspPost1"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            dspPost1DataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP scan timeout");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspTimeout");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dspTimeoutDataSource = (UA_DataSource)
        {
            .read = mci_get_dsp_timeout,
            .write = mci_set_dsp_timeout
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_TIMEOUT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspTimeout"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            dspTimeoutDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","DSP averaging");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","DspAveraging");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource dspAveragingDataSource = (UA_DataSource)
        {
            .read = mci_get_dsp_averaging,
            .write = mci_set_dsp_averaging
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_DSP_AVERAGING_ID),
            UA_NODEID_NUMERIC(1, LIBERA_DSP_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "DspAveraging"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            dspAveragingDataSource,
            NULL, NULL);

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
    
    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","attenuator setting");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","Attenuation");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calAttDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_attenuation,
            .write = mci_set_cal_attenuation
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ATT_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "Attenuation"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calAttDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel A calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KA");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calKADataSource = (UA_DataSource)
        {
            .read = mci_get_cal_ka,
            .write = mci_set_cal_ka
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KA_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KA"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calKADataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel B calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KB");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calKBDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_kb,
            .write = mci_set_cal_kb
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KB_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KB"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calKBDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel C calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KC");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calKCDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_kc,
            .write = mci_set_cal_kc
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KC_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KC"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calKCDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","channel D calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","KD");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calKDDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_kd,
            .write = mci_set_cal_kd
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_KD_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "KD"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calKDDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","position X calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearX");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calLinXDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_linx,
            .write = mci_set_cal_linx
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINX_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearX"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calLinXDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","position Y calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearY");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calLinYDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_liny,
            .write = mci_set_cal_liny
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINY_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearY"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calLinYDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","shape Q calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearQ");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calLinQDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_linq,
            .write = mci_set_cal_linq
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearQ"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calLinQDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","sum calibration factor");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","LinearS");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calLinSDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_lins,
            .write = mci_set_cal_lins
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_LINS_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "LinearS"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calLinSDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","position X offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetX");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calOffXDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_offx,
            .write = mci_set_cal_offx
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFX_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetX"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calOffXDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","position Y offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetY");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calOffYDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_offy,
            .write = mci_set_cal_offy
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFY_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetY"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calOffYDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","shape Q offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetQ");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calOffQDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_offq,
            .write = mci_set_cal_offq
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFQ_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetQ"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calOffQDataSource,
            NULL, NULL);

    attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en_US","sum offset");
    attr.displayName = UA_LOCALIZEDTEXT("en_US","OffsetS");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
    UA_DataSource calOffSDataSource = (UA_DataSource)
        {
            .read = mci_get_cal_offs,
            .write = mci_set_cal_offs
        };
    UA_Server_addDataSourceVariableNode(
            server,
            UA_NODEID_NUMERIC(1, LIBERA_CAL_OFFS_ID),
            UA_NODEID_NUMERIC(1, LIBERA_CAL_ID),
            UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
            UA_QUALIFIEDNAME(1, "OffsetS"),
            UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
            attr,
            calOffSDataSource,
            NULL, NULL);

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
