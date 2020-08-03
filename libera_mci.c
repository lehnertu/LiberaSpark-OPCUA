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

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "mci/mci.h"

#include "libera_mci.h"

// global variables for persistent access
mci::Node node_root;
mci::Node node_dev_freq;
mci::Node node_dsp_enable;
mci::Node node_dsp_thr1;
mci::Node node_dsp_pre;
mci::Node node_dsp_post1;
mci::Node node_dsp_timeout;
mci::Node node_dsp_averaging;
mci::Node node_maxadc;
mci::Node node_cal_attenuation;
mci::Node node_cal_ka;
mci::Node node_cal_kb;
mci::Node node_cal_kc;
mci::Node node_cal_kd;
mci::Node node_cal_linx;
mci::Node node_cal_liny;
mci::Node node_cal_linq;
mci::Node node_cal_lins;
mci::Node node_cal_offx;
mci::Node node_cal_offy;
mci::Node node_cal_offq;
mci::Node node_cal_offs;

int mci_error;

int mci_init()
{
    mci::Init();
    // connect to LiberaBase application running on local host
    node_root = mci::Connect();
    if (node_root.IsValid())
    {
        printf("MCI connect OK\n");
        mci_error = 0;
    } else {
        printf("MCI error : can't connect\n");
        mci_error = 1;
    }
    node_dev_freq = node_root.GetNode(mci::Tokenize("application.clock_info.adc_frequency"));
    if (!node_dev_freq.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.clock_info.adc_frequency\n");
    };
    node_dsp_enable = node_root.GetNode(mci::Tokenize("application.dsp.enable"));
    if (!node_dsp_enable.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.dsp.enable\n");
    };
    node_dsp_thr1 = node_root.GetNode(mci::Tokenize("application.dsp.bunch_thr1"));
    if (!node_dsp_thr1.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.dsp.bunch_thr1\n");
    };
    node_dsp_pre = node_root.GetNode(mci::Tokenize("application.dsp.pre_trigger"));
    if (!node_dsp_pre.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.dsp.pre_trigger\n");
    };
    node_dsp_post1 = node_root.GetNode(mci::Tokenize("application.dsp.post_trigger1"));
    if (!node_dsp_post1.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.dsp.post_trigger1\n");
    };
    node_dsp_timeout = node_root.GetNode(mci::Tokenize("application.dsp.scan_timeout"));
    if (!node_dsp_timeout.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.dsp.scan_timeout\n");
    };
    node_dsp_averaging = node_root.GetNode(mci::Tokenize("application.dsp.data_averaging"));
    if (!node_dsp_averaging.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.dsp.data_averaging\n");
    };
    node_maxadc = node_root.GetNode(mci::Tokenize("application.input.max_adc"));
    if (!node_maxadc.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.input.max_adc\n");
    };
    node_cal_attenuation = node_root.GetNode(mci::Tokenize("application.attenuation.att_id"));
    if (!node_cal_attenuation.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.attenuation.att_id\n");
    };
    node_cal_ka = node_root.GetNode(mci::Tokenize("application.calibration.ka"));
    if (!node_cal_ka.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.ka\n");
    };
    node_cal_kb = node_root.GetNode(mci::Tokenize("application.calibration.kb"));
    if (!node_cal_kb.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.kb\n");
    };
    node_cal_kc = node_root.GetNode(mci::Tokenize("application.calibration.kc"));
    if (!node_cal_kc.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.kc\n");
    };
    node_cal_kd = node_root.GetNode(mci::Tokenize("application.calibration.kd"));
    if (!node_cal_kd.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.kd\n");
    };
    node_cal_linx = node_root.GetNode(mci::Tokenize("application.calibration.linear.x.k"));
    if (!node_cal_linx.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.x.k\n");
    };
    node_cal_liny = node_root.GetNode(mci::Tokenize("application.calibration.linear.y.k"));
    if (!node_cal_liny.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.y.k\n");
    };
    node_cal_linq = node_root.GetNode(mci::Tokenize("application.calibration.linear.q.k"));
    if (!node_cal_linq.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.q.k\n");
    };
    node_cal_lins = node_root.GetNode(mci::Tokenize("application.calibration.linear.sum.k"));
    if (!node_cal_lins.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.sum.k\n");
    };
    node_cal_offx = node_root.GetNode(mci::Tokenize("application.calibration.linear.x.offs"));
    if (!node_cal_offx.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.x.offs\n");
    };
    node_cal_offy = node_root.GetNode(mci::Tokenize("application.calibration.linear.y.offs"));
    if (!node_cal_offy.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.y.offs\n");
    };
    node_cal_offq = node_root.GetNode(mci::Tokenize("application.calibration.linear.q.offs"));
    if (!node_cal_offq.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.q.offs\n");
    };
    node_cal_offs = node_root.GetNode(mci::Tokenize("application.calibration.linear.sum.offs"));
    if (!node_cal_offs.IsValid())
    {
        mci_error = 2;
        printf("MCI node error : application.calibration.linear.sum.offs\n");
    };
    return(mci_error);
}

int mci_shutdown()
{
    mci::Shutdown();
    printf("MCI shutdown OK\n");
    return(0);
}

UA_StatusCode mci_get_dev_freq(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_dev_freq.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.clock_info.adc_frequency\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_dsp_enable(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    bool val;
    if(node_dsp_enable.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Boolean*)(&val), &UA_TYPES[UA_TYPES_BOOLEAN]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.dsp.enable\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_dsp_enable(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_BOOLEAN]) && (data->value.data != 0))
    {
        bool val = *(bool*)data->value.data;
        if(node_dsp_enable.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.dsp.enable\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_dsp_enable\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_dsp_thr1(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_dsp_thr1.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.dsp.bunch_thr1\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_dsp_thr1(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_UINT32]) && (data->value.data != 0))
    {
        unsigned int val = *(unsigned int*)data->value.data;
        if(node_dsp_thr1.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.dsp.bunch_thr1\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_dsp_thr1\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_dsp_pre(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_dsp_pre.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.dsp.pre_trigger\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_dsp_pre(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_UINT32]) && (data->value.data != 0))
    {
        unsigned int val = *(unsigned int*)data->value.data;
        if(node_dsp_pre.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.dsp.pre_trigger\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_dsp_pre\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_dsp_post1(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_dsp_post1.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.dsp.post_trigger1\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_dsp_post1(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_UINT32]) && (data->value.data != 0))
    {
        unsigned int val = *(unsigned int*)data->value.data;
        if(node_dsp_post1.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.dsp.post_trigger1\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_dsp_post1\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_dsp_timeout(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_dsp_timeout.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.dsp.scan_timeout\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_dsp_timeout(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_UINT32]) && (data->value.data != 0))
    {
        unsigned int val = *(unsigned int*)data->value.data;
        if(node_dsp_timeout.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.dsp.scan_timeout\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_dsp_timeout\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_dsp_averaging(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_dsp_averaging.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.dsp.data_averaging\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_dsp_averaging(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_UINT32]) && (data->value.data != 0))
    {
        unsigned int val = *(unsigned int*)data->value.data;
        if(node_dsp_averaging.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.dsp.data_averaging\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_dsp_averaging\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_maxadc(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    unsigned int val;
    if(node_maxadc.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_UInt32*)(&val), &UA_TYPES[UA_TYPES_UINT32]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.input.max_adc\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_attenuation(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    int64_t val;
    if(node_cal_attenuation.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Int64*)(&val), &UA_TYPES[UA_TYPES_INT64]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.attenuation.att_id\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_attenuation(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_INT64]) && (data->value.data != 0))
    {
        int64_t val = *(int64_t*)data->value.data;
        if(node_cal_attenuation.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.attenuation.att_id\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_attenuation\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_ka(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_ka.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.ka\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_ka(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_ka.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.ka\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_ka\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_kb(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_kb.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.kb\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_kb(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_kb.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.kb\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_kb\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_kc(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_kc.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.kc\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_kc(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_kc.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.kc\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_kc\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_kd(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_kd.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.kd\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_kd(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_kd.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.kd\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_kd\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_linx(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_linx.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.x.k\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_linx(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_linx.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.x.k\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_linx\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_liny(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_liny.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.y.k\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_liny(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_liny.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.y.k\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_liny\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_linq(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_linq.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.q.k\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_linq(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_linq.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.q.k\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_linq\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_lins(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_lins.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.sum.k\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_lins(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_lins.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.sum.k\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_lins\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_offx(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_offx.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.x.offs\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_offx(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_offx.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.x.offs\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_offx\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_offy(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_offy.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.y.offs\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_offy(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_offy.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.y.offs\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_offy\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_offq(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_offq.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.q.offs\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_offq(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_offq.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.q.offs\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_offq\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_get_cal_offs(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    UA_Boolean sourceTimeStamp,
    const UA_NumericRange *range,
    UA_DataValue *dataValue)
{
    double val;
    if(node_cal_offs.GetValue(val))
    {
        UA_Variant_setScalarCopy(&dataValue->value, (UA_Double*)(&val), &UA_TYPES[UA_TYPES_DOUBLE]);
        dataValue->hasValue = true;
        return UA_STATUSCODE_GOOD;
    }
    else
    {
        mci_error = 3;
        printf("MCI value error : application.calibration.linear.sum.offs\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

UA_StatusCode mci_set_cal_offs(
    UA_Server *server,
    const UA_NodeId *sessionId, void *sessionContext,
    const UA_NodeId *nodeId, void *nodeContext,
    const UA_NumericRange *range,
    const UA_DataValue *data)
{
    if(data->hasValue && UA_Variant_isScalar(&data->value) && (data->value.type == &UA_TYPES[UA_TYPES_DOUBLE]) && (data->value.data != 0))
    {
        double val = *(double*)data->value.data;
        if(node_cal_offs.SetValue(val))
            return UA_STATUSCODE_GOOD;
        else
        {
            mci_error = 4;
            printf("MCI value error : application.calibration.linear.sum.offs\n");
            return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
        }
    }
    else
    {
		printf("data error : mci_set_cal_offs\n");
        return UA_STATUSCODE_UNCERTAINNOCOMMUNICATIONLASTUSABLEVALUE;
    }
}

