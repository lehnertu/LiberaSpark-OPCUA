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

/** @file libera_opcua.h
  OpcUaStreamServer : generic OPC UA data source handling routines
  Version 0.30 2016/08/08
  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
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


