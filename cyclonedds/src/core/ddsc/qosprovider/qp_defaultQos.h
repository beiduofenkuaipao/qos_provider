/*
 *                         Vortex OpenSplice
 *
 *   This software and documentation are Copyright 2006 to TO_YEAR ADLINK
 *   Technology Limited, its affiliated companies and licensors. All rights
 *   reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

#ifndef qp_defaultQos_H_
#define qp_defaultQos_H_

#if defined (__cplusplus)
extern "C" {
#endif

#include "c_typebase.h"

typedef c_sequence _DDS_octSeq;
typedef c_sequence _DDS_StringSeq;

enum _DDS_SchedulingClassQosPolicyKind {
    _DDS_SCHEDULE_DEFAULT,
    _DDS_SCHEDULE_TIMESHARING,
    _DDS_SCHEDULE_REALTIME
};
enum _DDS_SchedulingPriorityQosPolicyKind {
    _DDS_PRIORITY_RELATIVE,
    _DDS_PRIORITY_ABSOLUTE
};

enum _DDS_DurabilityQosPolicyKind {
    _DDS_VOLATILE_DURABILITY_QOS,
    _DDS_TRANSIENT_LOCAL_DURABILITY_QOS,
    _DDS_TRANSIENT_DURABILITY_QOS,
    _DDS_PERSISTENT_DURABILITY_QOS
};
enum _DDS_HistoryQosPolicyKind {
    _DDS_KEEP_LAST_HISTORY_QOS,
    _DDS_KEEP_ALL_HISTORY_QOS
};

enum _DDS_LivelinessQosPolicyKind {
    _DDS_AUTOMATIC_LIVELINESS_QOS,
    _DDS_MANUAL_BY_PARTICIPANT_LIVELINESS_QOS,
    _DDS_MANUAL_BY_TOPIC_LIVELINESS_QOS
};

enum _DDS_ReliabilityQosPolicyKind {
    _DDS_BEST_EFFORT_RELIABILITY_QOS,
    _DDS_RELIABLE_RELIABILITY_QOS
};

enum _DDS_DestinationOrderQosPolicyKind {
    _DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS,
    _DDS_BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS
};

enum _DDS_OwnershipQosPolicyKind {
    _DDS_SHARED_OWNERSHIP_QOS,
    _DDS_EXCLUSIVE_OWNERSHIP_QOS
};

enum _DDS_PresentationQosPolicyAccessScopeKind {
    _DDS_INSTANCE_PRESENTATION_QOS,
    _DDS_TOPIC_PRESENTATION_QOS,
    _DDS_GROUP_PRESENTATION_QOS
};

enum _DDS_InvalidSampleVisibilityQosPolicyKind {
    _DDS_NO_INVALID_SAMPLES,
    _DDS_MINIMUM_INVALID_SAMPLES,
    _DDS_ALL_INVALID_SAMPLES
};



















struct _DDS_UserDataQosPolicy {
    _DDS_octSeq value;
};

struct _DDS_EntityFactoryQosPolicy {
    c_bool autoenable_created_entities;
};

struct _DDS_SchedulingClassQosPolicy {
    enum _DDS_SchedulingClassQosPolicyKind kind;
};


struct _DDS_SchedulingPriorityQosPolicy {
    enum _DDS_SchedulingPriorityQosPolicyKind kind;
};


struct _DDS_SchedulingQosPolicy {
    struct _DDS_SchedulingClassQosPolicy scheduling_class;
    struct _DDS_SchedulingPriorityQosPolicy scheduling_priority_kind;
    c_long scheduling_priority;
};

struct _DDS_DomainParticipantQos {
    struct _DDS_UserDataQosPolicy user_data;
    struct _DDS_EntityFactoryQosPolicy entity_factory;
    struct _DDS_SchedulingQosPolicy watchdog_scheduling;
    struct _DDS_SchedulingQosPolicy listener_scheduling;
};

struct _DDS_NamedDomainParticipantQos {
    c_string name;
    struct _DDS_DomainParticipantQos domainparticipant_qos;
};

struct _DDS_TopicDataQosPolicy {
    _DDS_octSeq value;
};

struct _DDS_DurabilityQosPolicy {
    enum _DDS_DurabilityQosPolicyKind kind;
};

struct _DDS_Duration_t {
    c_long sec;
    c_ulong nanosec;
};


struct _DDS_DurabilityServiceQosPolicy {
    struct _DDS_Duration_t service_cleanup_delay;
    enum _DDS_HistoryQosPolicyKind history_kind;
    c_long history_depth;
    c_long max_samples;
    c_long max_instances;
    c_long max_samples_per_instance;
};

struct _DDS_DeadlineQosPolicy {
    struct _DDS_Duration_t period;
};

struct _DDS_LatencyBudgetQosPolicy {
    struct _DDS_Duration_t duration;
};

struct _DDS_LivelinessQosPolicy {
    enum _DDS_LivelinessQosPolicyKind kind;
    struct _DDS_Duration_t lease_duration;
};


struct _DDS_ReliabilityQosPolicy {
    enum _DDS_ReliabilityQosPolicyKind kind;
    struct _DDS_Duration_t max_blocking_time;
    c_bool synchronous;
};

struct _DDS_DestinationOrderQosPolicy {
    enum _DDS_DestinationOrderQosPolicyKind kind;
};

struct _DDS_HistoryQosPolicy {
    enum _DDS_HistoryQosPolicyKind kind;
    c_long depth;
};


struct _DDS_ResourceLimitsQosPolicy {
    c_long max_samples;
    c_long max_instances;
    c_long max_samples_per_instance;
};

struct _DDS_TransportPriorityQosPolicy {
    c_long value;
};

struct _DDS_LifespanQosPolicy {
    struct _DDS_Duration_t duration;
};

struct _DDS_OwnershipQosPolicy {
    enum _DDS_OwnershipQosPolicyKind kind;
};


struct _DDS_TopicQos {
    struct _DDS_TopicDataQosPolicy topic_data;
    struct _DDS_DurabilityQosPolicy durability;
    struct _DDS_DurabilityServiceQosPolicy durability_service;
    struct _DDS_DeadlineQosPolicy deadline;
    struct _DDS_LatencyBudgetQosPolicy latency_budget;
    struct _DDS_LivelinessQosPolicy liveliness;
    struct _DDS_ReliabilityQosPolicy reliability;
    struct _DDS_DestinationOrderQosPolicy destination_order;
    struct _DDS_HistoryQosPolicy history;
    struct _DDS_ResourceLimitsQosPolicy resource_limits;
    struct _DDS_TransportPriorityQosPolicy transport_priority;
    struct _DDS_LifespanQosPolicy lifespan;
    struct _DDS_OwnershipQosPolicy ownership;
};

struct _DDS_NamedTopicQos {
    c_string name;
    struct _DDS_TopicQos topic_qos;
};

struct _DDS_PresentationQosPolicy {
    enum _DDS_PresentationQosPolicyAccessScopeKind access_scope;
    c_bool coherent_access;
    c_bool ordered_access;
};

struct _DDS_PartitionQosPolicy {
    _DDS_StringSeq name;
};

struct _DDS_GroupDataQosPolicy {
    _DDS_octSeq value;
};


struct _DDS_PublisherQos {
    struct _DDS_PresentationQosPolicy presentation;
    struct _DDS_PartitionQosPolicy partition;
    struct _DDS_GroupDataQosPolicy group_data;
    struct _DDS_EntityFactoryQosPolicy entity_factory;
};

struct _DDS_NamedPublisherQos {
    c_string name;
    struct _DDS_PublisherQos publisher_qos;
};

struct _DDS_OwnershipStrengthQosPolicy {
    c_long value;
};

struct _DDS_WriterDataLifecycleQosPolicy {
    c_bool autodispose_unregistered_instances;
    struct _DDS_Duration_t autopurge_suspended_samples_delay;
    struct _DDS_Duration_t autounregister_instance_delay;
};

struct _DDS_DataWriterQos {
    struct _DDS_DurabilityQosPolicy durability;
    struct _DDS_DeadlineQosPolicy deadline;
    struct _DDS_LatencyBudgetQosPolicy latency_budget;
    struct _DDS_LivelinessQosPolicy liveliness;
    struct _DDS_ReliabilityQosPolicy reliability;
    struct _DDS_DestinationOrderQosPolicy destination_order;
    struct _DDS_HistoryQosPolicy history;
    struct _DDS_ResourceLimitsQosPolicy resource_limits;
    struct _DDS_TransportPriorityQosPolicy transport_priority;
    struct _DDS_LifespanQosPolicy lifespan;
    struct _DDS_UserDataQosPolicy user_data;
    struct _DDS_OwnershipQosPolicy ownership;
    struct _DDS_OwnershipStrengthQosPolicy ownership_strength;
    struct _DDS_WriterDataLifecycleQosPolicy writer_data_lifecycle;
};

struct _DDS_NamedDataWriterQos {
    c_string name;
    struct _DDS_DataWriterQos datawriter_qos;
};

struct _DDS_ShareQosPolicy {
    c_string name;
    c_bool enable;
};

struct _DDS_SubscriberQos {
    struct _DDS_PresentationQosPolicy presentation;
    struct _DDS_PartitionQosPolicy partition;
    struct _DDS_GroupDataQosPolicy group_data;
    struct _DDS_EntityFactoryQosPolicy entity_factory;
    struct _DDS_ShareQosPolicy share;
};

struct _DDS_NamedSubscriberQos {
    c_string name;
    struct _DDS_SubscriberQos subscriber_qos;
};

struct _DDS_TimeBasedFilterQosPolicy {
    struct _DDS_Duration_t minimum_separation;
};

struct _DDS_InvalidSampleVisibilityQosPolicy {
    enum _DDS_InvalidSampleVisibilityQosPolicyKind kind;
};

struct _DDS_ReaderDataLifecycleQosPolicy {
    struct _DDS_Duration_t autopurge_nowriter_samples_delay;
    struct _DDS_Duration_t autopurge_disposed_samples_delay;
    c_bool autopurge_dispose_all;
    c_bool enable_invalid_samples;
    struct _DDS_InvalidSampleVisibilityQosPolicy invalid_sample_visibility;
};

struct _DDS_SubscriptionKeyQosPolicy {
    c_bool use_key_list;
    _DDS_StringSeq key_list;
};

struct _DDS_ReaderLifespanQosPolicy {
    c_bool use_lifespan;
    struct _DDS_Duration_t duration;
};

struct _DDS_DataReaderQos {
    struct _DDS_DurabilityQosPolicy durability;
    struct _DDS_DeadlineQosPolicy deadline;
    struct _DDS_LatencyBudgetQosPolicy latency_budget;
    struct _DDS_LivelinessQosPolicy liveliness;
    struct _DDS_ReliabilityQosPolicy reliability;
    struct _DDS_DestinationOrderQosPolicy destination_order;
    struct _DDS_HistoryQosPolicy history;
    struct _DDS_ResourceLimitsQosPolicy resource_limits;
    struct _DDS_UserDataQosPolicy user_data;
    struct _DDS_OwnershipQosPolicy ownership;
    struct _DDS_TimeBasedFilterQosPolicy time_based_filter;
    struct _DDS_ReaderDataLifecycleQosPolicy reader_data_lifecycle;
    struct _DDS_SubscriptionKeyQosPolicy subscription_keys;
    struct _DDS_ReaderLifespanQosPolicy reader_lifespan;
    struct _DDS_ShareQosPolicy share;
};

struct _DDS_NamedDataReaderQos {
    c_string name;
    struct _DDS_DataReaderQos datareader_qos;
};






#define QP_LENGTH_UNLIMITED                        (-1)
#define QP_DURATION_INFINITE_SEC                   (0x7fffffff)
#define QP_DURATION_INFINITE_NSEC                  (0x7fffffffU)
#define QP_DURATION_INFINITE                       { QP_DURATION_INFINITE_SEC, QP_DURATION_INFINITE_NSEC }

#define QP_DURATION_ZERO_SEC                       (0)
#define QP_DURATION_ZERO_NSEC                      (0U)
#define QP_DURATION_ZERO                           { QP_DURATION_ZERO_SEC, QP_DURATION_ZERO_NSEC }

#define QP_DEFAULT_MAX_BLOCKING_TIME  {0, 100000000} /* 100ms */

#define QP_OwnershipStrengthQosPolicy_default_value { 0 }
#define QP_DurabilityQosPolicy_default_value { _DDS_VOLATILE_DURABILITY_QOS }
#define QP_DeadlineQosPolicy_default_value { QP_DURATION_INFINITE }
#define QP_LatencyBudgetQosPolicy_default_value { QP_DURATION_ZERO }
#define QP_LivelinessQosPolicy_default_value { _DDS_AUTOMATIC_LIVELINESS_QOS, QP_DURATION_INFINITE }
#define QP_ReliabilityQosPolicy_default_value { _DDS_BEST_EFFORT_RELIABILITY_QOS, QP_DEFAULT_MAX_BLOCKING_TIME, FALSE }
#define QP_ReliabilityQosPolicy_writer_default_value { _DDS_RELIABLE_RELIABILITY_QOS, QP_DEFAULT_MAX_BLOCKING_TIME, FALSE }
#define QP_DestinationOrderQosPolicy_default_value { _DDS_BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS }
#define QP_HistoryQosPolicy_default_value { _DDS_KEEP_LAST_HISTORY_QOS, 1 }
#define QP_ResourceLimitsQosPolicy_default_value { QP_LENGTH_UNLIMITED, QP_LENGTH_UNLIMITED, QP_LENGTH_UNLIMITED }
#define QP_TransportPriorityQosPolicy_default_value { 0 }
#define QP_LifespanQosPolicy_default_value { QP_DURATION_INFINITE }
#define QP_OwnershipQosPolicy_default_value { _DDS_SHARED_OWNERSHIP_QOS }
#define QP_PresentationQosPolicy_default_value { _DDS_INSTANCE_PRESENTATION_QOS, FALSE, FALSE }
#define QP_EntityFactoryQosPolicy_default_value { TRUE }
#define QP_WriterDataLifecycleQosPolicy_default_value { TRUE, QP_DURATION_INFINITE, QP_DURATION_INFINITE }
#define QP_SchedulingQosPolicy_default_value { { _DDS_SCHEDULE_DEFAULT }, { _DDS_PRIORITY_RELATIVE }, 0 }
#define QP_OctetSequence_default_value NULL
#define QP_StringSequence_default_value NULL
#define QP_UserDataQosPolicy_default_value { QP_OctetSequence_default_value }
#define QP_TopicDataQosPolicy_default_value { QP_OctetSequence_default_value }
#define QP_GroupDataQosPolicy_default_value { QP_OctetSequence_default_value }
#define QP_PartitionQosPolicy_default_value { QP_OctetSequence_default_value }
#define QP_ReaderDataLifecycleQosPolicy_default_value { QP_DURATION_INFINITE, QP_DURATION_INFINITE, FALSE, TRUE, { _DDS_MINIMUM_INVALID_SAMPLES } }
#define QP_TimeBasedFilterQosPolicy_default_value { QP_DURATION_ZERO }
#define QP_SubscriptionKeyQosPolicy_default_value { FALSE, QP_StringSequence_default_value }
#define QP_ReaderLifespanQosPolicy_default_value { FALSE, QP_DURATION_INFINITE }
#define QP_ShareQosPolicy_default_value { NULL, FALSE }
#define QP_ViewKeyQosPolicy_default_value { FALSE, NULL }
#define QP_DurabilityServiceQosPolicy_default_value { QP_DURATION_ZERO, _DDS_KEEP_LAST_HISTORY_QOS, 1, QP_LENGTH_UNLIMITED, QP_LENGTH_UNLIMITED, QP_LENGTH_UNLIMITED }

#define QP_DomainParticipantQos_default_value {    \
    QP_UserDataQosPolicy_default_value,            \
    QP_EntityFactoryQosPolicy_default_value,       \
    QP_SchedulingQosPolicy_default_value,          \
    QP_SchedulingQosPolicy_default_value           \
}

#define QP_TopicQos_default_value {                \
    QP_TopicDataQosPolicy_default_value,           \
    QP_DurabilityQosPolicy_default_value,          \
    QP_DurabilityServiceQosPolicy_default_value,   \
    QP_DeadlineQosPolicy_default_value,            \
    QP_LatencyBudgetQosPolicy_default_value,       \
    QP_LivelinessQosPolicy_default_value,          \
    QP_ReliabilityQosPolicy_default_value,         \
    QP_DestinationOrderQosPolicy_default_value,    \
    QP_HistoryQosPolicy_default_value,             \
    QP_ResourceLimitsQosPolicy_default_value,      \
    QP_TransportPriorityQosPolicy_default_value,   \
    QP_LifespanQosPolicy_default_value,            \
    QP_OwnershipQosPolicy_default_value            \
}

#define QP_PublisherQos_default_value {            \
    QP_PresentationQosPolicy_default_value,        \
    QP_PartitionQosPolicy_default_value,           \
    QP_GroupDataQosPolicy_default_value,           \
    QP_EntityFactoryQosPolicy_default_value        \
}

#define QP_SubscriberQos_default_value {           \
    QP_PresentationQosPolicy_default_value,        \
    QP_PartitionQosPolicy_default_value,           \
    QP_GroupDataQosPolicy_default_value,           \
    QP_EntityFactoryQosPolicy_default_value,       \
    QP_ShareQosPolicy_default_value                \
}

#define QP_DataReaderQos_default_value {           \
    QP_DurabilityQosPolicy_default_value,          \
    QP_DeadlineQosPolicy_default_value,            \
    QP_LatencyBudgetQosPolicy_default_value,       \
    QP_LivelinessQosPolicy_default_value,          \
    QP_ReliabilityQosPolicy_default_value,         \
    QP_DestinationOrderQosPolicy_default_value,    \
    QP_HistoryQosPolicy_default_value,             \
    QP_ResourceLimitsQosPolicy_default_value,      \
    QP_UserDataQosPolicy_default_value,            \
    QP_OwnershipQosPolicy_default_value,           \
    QP_TimeBasedFilterQosPolicy_default_value,     \
    QP_ReaderDataLifecycleQosPolicy_default_value, \
    QP_SubscriptionKeyQosPolicy_default_value,     \
    QP_ReaderLifespanQosPolicy_default_value,      \
    QP_ShareQosPolicy_default_value                \
}

#define QP_DataReaderViewQos_default_value {       \
    QP_SubscriptionKeyQosPolicy_default_value      \
}

#define QP_DataWriterQos_default_value {           \
    QP_DurabilityQosPolicy_default_value,          \
    QP_DeadlineQosPolicy_default_value,            \
    QP_LatencyBudgetQosPolicy_default_value,       \
    QP_LivelinessQosPolicy_default_value,          \
    QP_ReliabilityQosPolicy_writer_default_value,  \
    QP_DestinationOrderQosPolicy_default_value,    \
    QP_HistoryQosPolicy_default_value,             \
    QP_ResourceLimitsQosPolicy_default_value,      \
    QP_TransportPriorityQosPolicy_default_value,   \
    QP_LifespanQosPolicy_default_value,            \
    QP_UserDataQosPolicy_default_value,            \
    QP_OwnershipQosPolicy_default_value,           \
    QP_OwnershipStrengthQosPolicy_default_value,   \
    QP_WriterDataLifecycleQosPolicy_default_value  \
}

const struct _DDS_NamedDomainParticipantQos qp_NamedDomainParticipantQos_default = {NULL, QP_DomainParticipantQos_default_value};
const struct _DDS_NamedTopicQos qp_NamedTopicQos_default = {NULL, QP_TopicQos_default_value};
const struct _DDS_NamedPublisherQos qp_NamedPublisherQos_default = {NULL, QP_PublisherQos_default_value};
const struct _DDS_NamedDataWriterQos qp_NamedDataWriterQos_default = {NULL, QP_DataWriterQos_default_value};
const struct _DDS_NamedSubscriberQos qp_NamedSubscriberQos_default = {NULL, QP_SubscriberQos_default_value};
const struct _DDS_NamedDataReaderQos qp_NamedDataReaderQos_default = {NULL, QP_DataReaderQos_default_value};






#if defined (__cplusplus)
}
#endif

#endif /* qp_defaultQos_H_ */
