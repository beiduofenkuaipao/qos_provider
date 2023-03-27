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
#include <cstdlib>
#include <iostream>
#include <chrono>
#include <thread>

/* Include the C++ DDS API. */
#include "dds/dds.hpp"

/* Include data type and specific traits to be used with the C++ DDS API. */
#include "HelloWorldData.hpp"

using namespace org::eclipse::cyclonedds;

int main() {
    try {
        std::cout << "=== [Publisher] Create writer." << std::endl;
        #if 1

        dds::core::QosProvider qosProvider("file:///home/dong/work/10_cyc_qos_test_4/cyclonedds-cxx/examples/helloworld/test_qos.xml","BarQosProfile");
        dds::domain::DomainParticipant participant(domain::default_id(),qosProvider.participant_qos());
        dds::topic::Topic<HelloWorldData::Msg> topic(participant, "HelloWorldData_Msg",qosProvider.topic_qos());
        dds::pub::Publisher publisher(participant,qosProvider.publisher_qos());
        dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic,qosProvider.datawriter_qos("::BarQosProfile::Persistent"));
        std::cout << qosProvider.datawriter_qos("::BarQosProfile::Persistent").policy<dds::core::policy::Reliability>().kind() << std::endl;
        #else

        dds::domain::DomainParticipant participant(domain::default_id());
        dds::topic::Topic<HelloWorldData::Msg> topic(participant, "HelloWorldData_Msg");
        dds::pub::qos::PublisherQos pqos;
        pqos.policy(dds::core::policy::Partition("Hellow example"));
        dds::pub::Publisher publisher(participant, pqos);
        dds::pub::qos::DataWriterQos wqos;
        wqos.policy(
            dds::core::policy::Reliability(
            dds::core::policy::ReliabilityKind::Type::RELIABLE,
            dds::core::Duration::from_secs(10)));
        wqos.policy(
            dds::core::policy::History(
            dds::core::policy::HistoryKind::Type::KEEP_ALL,
            0));
        dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic, wqos);

        #endif

        
        
        /* For this example, we'd like to have a subscriber to actually read
         * our message. This is not always necessary. Also, the way it is
         * done here is just to illustrate the easiest way to do so. It isn't
         * really recommended to do a wait in a polling loop, however.
         * Please take a look at Listeners and WaitSets for much better
         * solutions, albeit somewhat more elaborate ones. */


        std::cout << "=== [Publisher] Waiting for subscriber." << std::endl;
        while (writer.publication_matched_status().current_count() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        /* Create a message to write. */
        HelloWorldData::Msg msg(1, "Hello World");

        /* Write the message. */
        std::cout << "=== [Publisher] Write sample." << std::endl;
        

        /* With a normal configuration (see dds::pub::qos::DataWriterQos
         * for various different writer configurations), deleting a writer will
         * dispose all its related message.
         * Wait for the subscriber to have stopped to be sure it received the
         * message. Again, not normally necessary and not recommended to do
         * this in a polling loop. */
        std::cout << "=== [Publisher] Waiting for sample to be accepted." << std::endl;
        while (writer.publication_matched_status().current_count() > 0) {
            writer.write(msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    catch (const dds::core::Exception& e) {
        std::cerr << "=== [Publisher] Exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "=== [Publisher] Done." << std::endl;

    return EXIT_SUCCESS;
}
