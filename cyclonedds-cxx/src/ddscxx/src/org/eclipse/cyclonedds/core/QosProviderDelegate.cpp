/*
 * Copyright(c) 2006 to 2020 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

/**
 * @file
 */

#include <dds/domain/qos/DomainParticipantQos.hpp>
#include <dds/pub/qos/DataWriterQos.hpp>
#include <dds/pub/qos/PublisherQos.hpp>
#include <dds/sub/qos/DataReaderQos.hpp>
#include <dds/sub/qos/SubscriberQos.hpp>
#include <dds/topic/qos/TopicQos.hpp>
#include <org/eclipse/cyclonedds/core/QosProviderDelegate.hpp>
#include <string>
#include "/home/dong/work/09_cyc_qos_test_3/cyclonedds/src/core/ddsc/qosprovider/test_provider.h"
// FIXME
#include <iostream>
using namespace std;

/**
 * @brief
 * The QosProvider API allows users to specify the QoS settings of their DCPS
 * entities outside of application code in XML.
 *
 * The QosProvider is delivered as part of the DCPS API of Vortex OpenSplice.
 */

// Initialization of the QosProvider will fail under the following conditions:<br>
// - No uri is provided.
// - The resource pointed to by uri cannot be found.
// - The content of the resource pointed to by uri is malformed (e.g., malformed XML).
// When initialization fails (for example, due to a parse error or when the resource


const C_STRUCT(cmn_qosProviderInputAttr) org::eclipse::cyclonedds::core::QosProviderDelegate::QosProviderDelegate::qosProviderAttr = {
    { &QosProviderDelegate::named_qos__copyOut<struct _DDS_NamedDomainParticipantQos, dds::domain::qos::DomainParticipantQos>  },
    { &QosProviderDelegate::named_qos__copyOut<struct _DDS_NamedTopicQos,             dds::topic::qos::TopicQos             >  },
    { &QosProviderDelegate::named_qos__copyOut<struct _DDS_NamedSubscriberQos,        dds::sub::qos::SubscriberQos          >  },
    { &QosProviderDelegate::named_qos__copyOut<struct _DDS_NamedDataReaderQos,        dds::sub::qos::DataReaderQos          >  },
    { &QosProviderDelegate::named_qos__copyOut<struct _DDS_NamedPublisherQos,         dds::pub::qos::PublisherQos           >  },
    { &QosProviderDelegate::named_qos__copyOut<struct _DDS_NamedDataWriterQos,        dds::pub::qos::DataWriterQos          >  }
};

org::eclipse::cyclonedds::core::QosProviderDelegate::QosProviderDelegate(const std::string& uri, const std::string& id)
{
    //ISOCPP_REPORT_STACK_NC_BEGIN();
    if(uri.empty()) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR, "Invalid Qos Provider URI (empty)");
    }

    this->qosProvider = cmn_qosProviderNew(uri.c_str(), id.c_str(), &this->qosProviderAttr);
    if (this->qosProvider == NULL) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "QoSProvider not properly instantiated");
    }
}

org::eclipse::cyclonedds::core::QosProviderDelegate::~QosProviderDelegate()
{
    cmn_qosProviderFree(this->qosProvider);
}

/**
 * Resolves the DomainParticipantQos identified by the id from the uri this
 * QosProvider is associated with.
 *
 * @param id        The fully-qualified name that identifies a QoS within the uri
 *                  associated with the QosProvider or a name that identifies a QoS within the
 *                  uri associated with the QosProvider instance relative to its default QoS
 *                  profile. Id’s starting with ‘::’ are interpreted as fully-qualified names and all
 *                  others are interpreted as names relative to the default QoS profile of the
 *                  QosProvider instance. When id is NULL it is interpreted as a non-named QoS
 *                  within the default QoS profile associated with the QosProvider.
 * @return DomainParticipantQos from the given URI (and profile) using the id
 * @throws dds::core::Error
 *                  An internal error has occurred.
 * @throws dds::core::PreconditionNotMetError
 *                  If no DomainParticipantQos that matches the provided id can be
 *                  found within the uri associated with the QosProvider.
 * @throws dds::core::OutOfResourcesError
 *                  The Data Distribution Service ran out of resources to
 *                  complete this operation.
 */
dds::domain::qos::DomainParticipantQos org::eclipse::cyclonedds::core::QosProviderDelegate::participant_qos(
    const char* id)
{
    cmn_qpResult qpResult;
    dds::domain::qos::DomainParticipantQos qos;
    qpResult = cmn_qosProviderGetParticipantQos(this->qosProvider, id, &qos);
    //ISOCPP_QP_RESULT_CHECK_AND_THROW(qpResult);
    return qos;
}

dds::topic::qos::TopicQos org::eclipse::cyclonedds::core::QosProviderDelegate::topic_qos(const char* id)
{
    cmn_qpResult qpResult;
    dds::topic::qos::TopicQos qos;
    qpResult = cmn_qosProviderGetTopicQos(this->qosProvider, id, &qos);
    //ISOCPP_QP_RESULT_CHECK_AND_THROW(qpResult);
    return qos;
}

dds::sub::qos::SubscriberQos org::eclipse::cyclonedds::core::QosProviderDelegate::subscriber_qos(const char* id)
{
    cmn_qpResult qpResult;
    dds::sub::qos::SubscriberQos qos;
    qpResult = cmn_qosProviderGetSubscriberQos(this->qosProvider, id, &qos);
    //ISOCPP_QP_RESULT_CHECK_AND_THROW(qpResult);
    return qos;
}

dds::sub::qos::DataReaderQos org::eclipse::cyclonedds::core::QosProviderDelegate::datareader_qos(const char* id)
{
    cmn_qpResult qpResult;
    dds::sub::qos::DataReaderQos qos;
    qpResult = cmn_qosProviderGetDataReaderQos(this->qosProvider, id, &qos);
    //ISOCPP_QP_RESULT_CHECK_AND_THROW(qpResult);
    return qos;
}

dds::pub::qos::PublisherQos org::eclipse::cyclonedds::core::QosProviderDelegate::publisher_qos(const char* id)
{
    cmn_qpResult qpResult;
    dds::pub::qos::PublisherQos qos;
    qpResult = cmn_qosProviderGetPublisherQos(this->qosProvider, id, &qos);
    //ISOCPP_QP_RESULT_CHECK_AND_THROW(qpResult);
    return qos;
}

dds::pub::qos::DataWriterQos org::eclipse::cyclonedds::core::QosProviderDelegate::datawriter_qos(const char* id)
{
    cmn_qpResult qpResult;
    dds::pub::qos::DataWriterQos qos;
    qpResult = cmn_qosProviderGetDataWriterQos(this->qosProvider, id, &qos);
    std::cout<<"QosProviderDelegate::datawriter_qos qpResult = "<< qpResult <<std::endl;
    //ISOCPP_QP_RESULT_CHECK_AND_THROW(qpResult);
    return qos;
}

template <typename FROM, typename TO>
void
org::eclipse::cyclonedds::core::QosProviderDelegate::named_qos__copyOut(
    void *from,
    void *to)
{
    FROM *named_qos = (FROM*)from;
    TO   *dds_qos   = (TO*)to;
    /* Do the copy. */
    dds_qos->delegate().named_qos(*named_qos);
}
