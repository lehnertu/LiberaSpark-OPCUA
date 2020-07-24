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

UA_StatusCode readBool(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Boolean*)nodeContext, &UA_TYPES[UA_TYPES_BOOLEAN]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode writeBool(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(UA_Variant_isScalar(&(data->value)) && data->value.type == &UA_TYPES[UA_TYPES_BOOLEAN] && data->value.data){
        *(UA_Boolean*)nodeContext = *(UA_Boolean*)data->value.data;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode readDouble(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)nodeContext, &UA_TYPES[UA_TYPES_DOUBLE]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode writeDouble(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(UA_Variant_isScalar(&(data->value)) && data->value.type == &UA_TYPES[UA_TYPES_DOUBLE] && data->value.data){
        *(UA_Double*)nodeContext = *(UA_Double*)data->value.data;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode readInt32(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    UA_Variant_setScalarCopy(&dataValue->value, (UA_Int32*)nodeContext, &UA_TYPES[UA_TYPES_INT32]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode writeInt32(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(UA_Variant_isScalar(&(data->value)) && data->value.type == &UA_TYPES[UA_TYPES_INT32] && data->value.data){
        *(UA_Int32*)nodeContext = *(UA_Int32*)data->value.data;
    }
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode readUInt32(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)nodeContext, &UA_TYPES[UA_TYPES_UINT32]);
    dataValue->hasValue = true;
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode writeUInt32(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    printf("writeUInt32\n");
    if(UA_Variant_isScalar(&(data->value)) && data->value.type == &UA_TYPES[UA_TYPES_UINT32] && data->value.data){
        *(UA_UInt32*)nodeContext = *(UA_UInt32*)data->value.data;
        printf("good\n");
    }
    return UA_STATUSCODE_GOOD;
}


