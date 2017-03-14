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
