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
