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

/** @file libera_mci.h
  OpcUaStreamServer : MCI access
  Version 0.30 2016/08/08
  @author U. Lehnert, Helmholtz-Zentrum Dresden-Rossendorf
 */

#ifndef LIBERAMCI_H
#define LIBERAMCI_H

#include "open62541.h"       // the OPC UA library

#ifdef __cplusplus
extern "C" {
#endif

// initialize MCI connection
int mci_init();
// shutdown MCI connection
int mci_shutdown();

// OPC-UA data source routines

UA_StatusCode mci_get_dev_freq(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);

UA_StatusCode mci_get_dsp_enable(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_dsp_enable(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_dsp_thr1(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_dsp_thr1(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_dsp_pre(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_dsp_pre(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_dsp_post1(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_dsp_post1(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_dsp_timeout(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_dsp_timeout(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_dsp_averaging(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_dsp_averaging(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_maxadc(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);

UA_StatusCode mci_get_cal_attenuation(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_attenuation(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_ka(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_ka(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_kb(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_kb(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_kc(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_kc(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_kd(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_kd(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_linx(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_linx(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_liny(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_liny(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_linq(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_linq(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_lins(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_lins(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_offx(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_offx(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_offy(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_offy(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_offq(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_offq(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

UA_StatusCode mci_get_cal_offs(
    void *handle, const UA_NodeId nodeid,
    UA_Boolean sourceTimeStamp, const UA_NumericRange *range, UA_DataValue *dataValue);
UA_StatusCode mci_set_cal_offs(
    void *handle, const UA_NodeId nodeid, const UA_Variant *data, const UA_NumericRange *range);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
