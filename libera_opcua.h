#ifndef LIBERAOPCUA_H
#define LIBERAOPCUA_H

#include "open62541.h"       // the OPC UA library

#ifdef __cplusplus
extern "C" {
#endif

/***********************************/
/* generic handling routines       */
/* for OPC-UA data sources         */
/***********************************/

UA_StatusCode
readBool( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
          const UA_NumericRange *range, UA_DataValue *dataValue);

UA_StatusCode
writeBool(void *handle, const UA_NodeId nodeid,
          const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode
readDouble( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue);

UA_StatusCode
writeDouble(void *handle, const UA_NodeId nodeid,
            const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode
readInt32( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *dataValue);

UA_StatusCode
writeInt32(void *handle, const UA_NodeId nodeid,
           const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode
readUInt32( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue);

UA_StatusCode
writeUInt32(void *handle, const UA_NodeId nodeid,
            const UA_Variant *data, const UA_NumericRange *range);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
