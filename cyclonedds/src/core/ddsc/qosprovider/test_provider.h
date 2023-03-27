#ifndef TEST_PROVIDER_H_
#define TEST_PROVIDER_H_

#if defined (__cplusplus)
extern "C" {
#endif
#include "dds/export.h"
#include "c_typebase.h"
#include "c_mmbase.h"
#include "c_base.h"

#define QOS_DATABASE_SIZE (2 * 1024 * 1024)
#define DDS_TAG                 "dds"
#define PROFILE_TAG             "qos_profile"
#define DPQOS_TAG               "domainparticipant_qos"
#define TQOS_TAG                "topic_qos"
#define PUBQOS_TAG              "publisher_qos"
#define SUBQOS_TAG              "subscriber_qos"
#define DWQOS_TAG               "datawriter_qos"
#define DRQOS_TAG               "datareader_qos"

#if 0
static int level;
#define QP_TRACE(t) t
#else
#define QP_TRACE(t)
#endif

typedef enum {
    QP_SCOPE_NONE,
    QP_SCOPE_DDS,
    QP_SCOPE_PROFILE,
    QP_SCOPE_QOSPOLICY
} qp_parseScope;

C_CLASS(cmn_qosProvider);
C_STRUCT(cmn_qosProvider) {
     c_base                      baseAddr;
     c_char *                    defaultProfile;
     cf_element                  rootElement;
     C_STRUCT (qp_entityAttr)    dpQosAttr;
     C_STRUCT (qp_entityAttr)    tQosAttr;
     C_STRUCT (qp_entityAttr)    pubQosAttr;
     C_STRUCT (qp_entityAttr)    subQosAttr;
     C_STRUCT (qp_entityAttr)    dwQosAttr;
     C_STRUCT (qp_entityAttr)    drQosAttr;
};

C_CLASS(qp_parseContext);
C_STRUCT(qp_parseContext) {
    cmn_qosProvider              qosProvider;
    c_field                     currentField;
    c_type                      currentFieldType;
    c_object                    qosSample;
    ut_table                    qosTable;
    qp_parseScope               scope;
    const char *                name;
};

void cmn_qosProviderFree(cmn_qosProvider _this);
cmn_qosProvider cmn_qosProviderNew(const c_char *uri,const c_char *profile,const C_STRUCT(cmn_qosProviderInputAttr) *attr);
cmn_qpResult processElement(cf_element element,C_STRUCT(qp_parseContext) ctx);
cmn_qpResult processContainedElements(c_iter elements,C_STRUCT(qp_parseContext) ctx);
cmn_qpResult processElementData(cf_data data,C_STRUCT(qp_parseContext) ctx);
const char * substituteConstants(const char *xmlValue);
cmn_qpResult loadQosProviderQosAttributes(cmn_qosProvider _this,const C_STRUCT(cmn_qosProviderInputAttr) *attr);
os_equality cmn_qosCompareByName (void *o1,void *o2,void *unused);
void cmn_qosTablefreeKey (void *o,void *unused);
void cmn_qosTablefreeData (void *o,void *unused);
cmn_qpResult checkQosProviderAttrIsSane (const C_STRUCT(cmn_qosProviderInputAttr) *attr);
cmn_qpResult processContainedAttributes(c_iter attributes,qp_parseContext ctx);
cmn_qpResult processAttribute(cf_attribute attribute,qp_parseContext ctx);
c_value cf_attributeValue(cf_attribute attribute);
cmn_qpResult prepareQosSample(const char *qosName,qp_entityAttr attr,qp_parseContext ctx);
cmn_qpResult cmn_qosProviderGetParticipantQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
cmn_qpResult cmn_qosProviderGetTopicQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
cmn_qpResult cmn_qosProviderGetSubscriberQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
cmn_qpResult cmn_qosProviderGetDataReaderQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
cmn_qpResult cmn_qosProviderGetPublisherQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
cmn_qpResult cmn_qosProviderGetDataWriterQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);

c_object cmn_qosTableLookup(ut_table t,const c_char *defaultProfile,const c_char *id);
c_metaObject __dds_namedQosTypes_DDS__load (c_base base);
c_metaObject __DDS_NamedDomainParticipantQos__load (c_base base);
c_metaObject __DDS_NamedPublisherQos__load (c_base base);
c_metaObject __DDS_NamedSubscriberQos__load (c_base base);
c_metaObject __DDS_NamedTopicQos__load (c_base base);
c_metaObject __DDS_NamedDataWriterQos__load (c_base base);
c_metaObject __DDS_NamedDataReaderQos__load (c_base base);
void qp_NamedDataReaderQos__shallowClone(void *_from,void *_to);

void qp_NamedDataWriterQos__shallowClone(void *_from,void *_to);

void qp_NamedSubscriberQos__shallowClone(void *_from,void *_to);

void qp_NamedPublisherQos__shallowClone(void *_from,void *_to);

void qp_NamedTopicQos__shallowClone(void *_from,void *_to);

void qp_NamedDomainParticipantQos__shallowClone( void *_from,void *_to);                                        
c_metaObject __DDS_DomainParticipantQos__load (c_base base);

c_metaObject __dds_dcps_builtintopics_DDS__load (c_base base);

c_metaObject __dds_builtinTopics_DDS__load (c_base base);

c_metaObject __DDS_UserDataQosPolicy__load (c_base base);

c_metaObject __DDS_octSeq__load (c_base base);

c_metaObject __DDS_EntityFactoryQosPolicy__load (c_base base);

c_metaObject __DDS_SchedulingQosPolicy__load (c_base base);

c_metaObject __DDS_SchedulingClassQosPolicy__load (c_base base);

c_metaObject __DDS_SchedulingClassQosPolicyKind__load (c_base base);

c_metaObject __DDS_SchedulingPriorityQosPolicy__load (c_base base);

c_metaObject __DDS_SchedulingPriorityQosPolicyKind__load (c_base base);

c_metaObject __DDS_DataReaderQos__load (c_base base);
c_metaObject
__DDS_DurabilityQosPolicy__load (
    c_base base);

c_metaObject
__DDS_DurabilityQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_DeadlineQosPolicy__load (
    c_base base);


c_metaObject
__DDS_Duration_t__load (
    c_base base);

c_metaObject
__DDS_LatencyBudgetQosPolicy__load (
    c_base base);


c_metaObject
__DDS_LivelinessQosPolicy__load (
    c_base base);

c_metaObject
__DDS_LivelinessQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_ReliabilityQosPolicy__load (
    c_base base);


c_metaObject
__DDS_ReliabilityQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_DestinationOrderQosPolicy__load (
    c_base base);

c_metaObject
__DDS_DestinationOrderQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_HistoryQosPolicy__load (
    c_base base);

c_metaObject
__DDS_HistoryQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_ResourceLimitsQosPolicy__load (
    c_base base);

c_metaObject
__DDS_OwnershipQosPolicy__load (
    c_base base);

c_metaObject
__DDS_OwnershipQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_TimeBasedFilterQosPolicy__load (
    c_base base);

c_metaObject
__DDS_ReaderDataLifecycleQosPolicy__load (
    c_base base);

c_metaObject
__DDS_InvalidSampleVisibilityQosPolicy__load (
    c_base base);

c_metaObject
__DDS_InvalidSampleVisibilityQosPolicyKind__load (
    c_base base);

c_metaObject
__DDS_SubscriptionKeyQosPolicy__load (
    c_base base);

c_metaObject
__DDS_StringSeq__load (
    c_base base);


c_metaObject
__DDS_ReaderLifespanQosPolicy__load (
    c_base base);


c_metaObject
__DDS_ShareQosPolicy__load (
    c_base base);


c_metaObject
__DDS_PublisherQos__load (
    c_base base);

c_metaObject
__DDS_PresentationQosPolicy__load (
    c_base base);

c_metaObject
__DDS_PresentationQosPolicyAccessScopeKind__load (
    c_base base);

c_metaObject
__DDS_PartitionQosPolicy__load (
    c_base base);


c_metaObject
__DDS_GroupDataQosPolicy__load (
    c_base base);

c_metaObject
__DDS_SubscriberQos__load (
    c_base base);

c_metaObject
__DDS_TopicQos__load (
    c_base base);

c_metaObject
__DDS_TopicDataQosPolicy__load (
    c_base base);

c_metaObject
__DDS_DurabilityServiceQosPolicy__load (
    c_base base);

c_metaObject
__DDS_TransportPriorityQosPolicy__load (
    c_base base);


c_metaObject
__DDS_LifespanQosPolicy__load (
    c_base base);


c_metaObject
__DDS_DataWriterQos__load (
    c_base base);

c_metaObject
__DDS_OwnershipStrengthQosPolicy__load (
    c_base base);

c_metaObject
__DDS_WriterDataLifecycleQosPolicy__load (
    c_base base);




DDS_EXPORT cmn_qosProvider cmn_qosProviderNew(const c_char *uri,const c_char *profile,const C_STRUCT(cmn_qosProviderInputAttr) *attr);
DDS_EXPORT void cmn_qosProviderFree(cmn_qosProvider _this);
DDS_EXPORT cmn_qpResult cmn_qosProviderGetParticipantQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
DDS_EXPORT cmn_qpResult cmn_qosProviderGetTopicQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
DDS_EXPORT cmn_qpResult cmn_qosProviderGetSubscriberQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
DDS_EXPORT cmn_qpResult cmn_qosProviderGetDataReaderQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
DDS_EXPORT cmn_qpResult cmn_qosProviderGetPublisherQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);
DDS_EXPORT cmn_qpResult cmn_qosProviderGetDataWriterQos(cmn_qosProvider _this,const c_char *id,c_voidp qos);

#if defined (__cplusplus)
}
#endif

#endif 