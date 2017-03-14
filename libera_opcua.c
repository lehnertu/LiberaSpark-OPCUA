/*

  OpcUaStreamServer : OPC-UA variable handling

  Version 0.30 2016/08/08

  U. Lehnert
  Helmholtz-Zentrum Dresden-Rossendorf

*/

#include "libera_opcua.h"

UA_StatusCode
readBool( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
          const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Boolean*)handle, &UA_TYPES[UA_TYPES_BOOLEAN]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeBool(void *handle, const UA_NodeId nodeid,
          const UA_Variant *data, const UA_NumericRange *range) {
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_BOOLEAN] && data->data){
        *(UA_Boolean*)handle = *(UA_Boolean*)data->data;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
readDouble( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)handle, &UA_TYPES[UA_TYPES_DOUBLE]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeDouble(void *handle, const UA_NodeId nodeid,
            const UA_Variant *data, const UA_NumericRange *range) {
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_DOUBLE] && data->data){
        *(UA_Double*)handle = *(UA_Double*)data->data;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
readInt32( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
           const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Int32*)handle, &UA_TYPES[UA_TYPES_INT32]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeInt32(void *handle, const UA_NodeId nodeid,
           const UA_Variant *data, const UA_NumericRange *range) {
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_INT32] && data->data){
        *(UA_Int32*)handle = *(UA_Int32*)data->data;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
readUInt32( void *handle, const UA_NodeId nodeid, UA_Boolean sourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *dataValue) {
    dataValue->hasValue = true;
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)handle, &UA_TYPES[UA_TYPES_UINT32]);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode
writeUInt32(void *handle, const UA_NodeId nodeid,
            const UA_Variant *data, const UA_NumericRange *range) {
    if(UA_Variant_isScalar(data) && data->type == &UA_TYPES[UA_TYPES_UINT32] && data->data){
        *(UA_UInt32*)handle = *(UA_UInt32*)data->data;
    }
    return UA_STATUSCODE_GOOD;
}


