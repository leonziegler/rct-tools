/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   AnotherRctROSBridge.cpp
 * Author: michael
 *
 * Created on 17. Dezember 2015, 09:11
 */

#include <cstdlib>

#include <boost/lockfree/queue.hpp>
#include <boost/date_time/posix_time/ptime.hpp>

#include <rsc/logging/Logger.h>
#include <rsc/misc/SignalWaiter.h>

#include <rct/impl/TransformCommunicator.h>
#include <rct/impl/TransformCommRsb.h>
#include <rct/impl/TransformCommRos.h>

using namespace std;
using namespace rct;
using namespace rsc::misc;

class TransformWrapper {
public:
    Transform* transform;
    bool isStatic;
};

template <class queue_t>
class Handler : public TransformListener {
public:
    typedef boost::shared_ptr< Handler<queue_t> > Ptr;

    Handler(queue_t& queue, std::string authorityToIgnore) :
    queue(queue), logger(rsc::logging::Logger::getLogger("rct.anotherrctrosbridge.Handler")),
    authorityToIgnore(authorityToIgnore) {
    }

    virtual ~Handler() {
    }

    void newTransformAvailable(const Transform& transform, bool isStatic) {

        TransformWrapper wrappedTransform;

        wrappedTransform.isStatic = isStatic;
        wrappedTransform.transform = new Transform(transform);

        while (!queue.push(wrappedTransform)) {
            TransformWrapper discardedTransform;
            queue.pop(discardedTransform);
        }
    }
private:
    std::string authorityToIgnore;
    queue_t& queue;
    rsc::logging::LoggerPtr logger;
};

template<int timeBeweenTicksMS = 20, int queueBounds = 10000 >
class CommunicatorConnector {
public:
    typedef boost::lockfree::queue< TransformWrapper, boost::lockfree::capacity<queueBounds> > queue_t;

    CommunicatorConnector(TransformCommunicator::Ptr fromCommunicator, TransformCommunicator::Ptr toCommunicator,
            std::string logname) :
    fromCommunicator(fromCommunicator), toCommunicator(toCommunicator),
    logger(rsc::logging::Logger::getLogger("rct.anotherrctrosbridge." + logname)) {
        fromCommunicator->addTransformListener(typename Handler<queue_t>::Ptr(new Handler<queue_t>(queue, "none")));
        nextTickTime = boost::posix_time::microsec_clock::universal_time();

        boost::thread temp(boost::bind(&CommunicatorConnector<timeBeweenTicksMS, queueBounds>::publishThread, this));
        publisher.swap(temp);
    }

    void stop() {
        publisher.interrupt();
    }

private:

    void publishThread() {

        unsigned long numTransformsRelayed = 0;
        unsigned long numCycles = 0;
        while (true) {
            
            vector<Transform> dynamicTransforms;
            vector<Transform> staticTransforms;

            TransformWrapper transform;
            while (queue.pop(transform)) {
                if (transform.transform->getAuthority() != fromCommunicator->getAuthorityName()) {
                    if (transform.isStatic) {
                        staticTransforms.push_back(*transform.transform);
//                        toCommunicator->sendTransform(staticTransforms.back(), rct::STATIC);
                    } else {
                        dynamicTransforms.push_back(*transform.transform);
//                        toCommunicator->sendTransform(dynamicTransforms.back(), rct::DYNAMIC);
                    }
                } else {
                    RSCDEBUG(logger, "ignored transform due to authority!");
                }
                delete transform.transform;
            }

            numTransformsRelayed += dynamicTransforms.size();
            numTransformsRelayed += staticTransforms.size();

            if (dynamicTransforms.size() > 0) {
                RSCTRACE(logger, boost::str(boost::format("Sending %1% dynamic transforms!") % dynamicTransforms.size()));
                toCommunicator->sendTransform(dynamicTransforms, rct::DYNAMIC);
            }
            if (staticTransforms.size() > 0) {
                RSCTRACE(logger, boost::str(boost::format("Sending %1% static transforms!") % staticTransforms.size()));
                toCommunicator->sendTransform(staticTransforms, rct::STATIC);
            }

            nextTickTime += boost::posix_time::millisec(timeBeweenTicksMS);
            boost::this_thread::sleep(nextTickTime);
            numCycles++;
            RSCDEBUG(logger, boost::str(boost::format("Have sent %1% transforms in %2% cycles") % numTransformsRelayed % numCycles));
        }
    }

    rsc::logging::LoggerPtr logger;
    boost::thread publisher;
    TransformCommunicator::Ptr fromCommunicator;
    TransformCommunicator::Ptr toCommunicator;
    queue_t queue;
    boost::posix_time::ptime nextTickTime;
};

/*
 * 
 */
int main(int argc, char** argv) {
    ros::init(argc, argv, "rctrosbridge");
    ros::AsyncSpinner spinner(4);
    spinner.start();

    initSignalWaiter();

    TransformCommunicator::Ptr commRsb;
    TransformerConfig configRsb;
    configRsb.setCommType(TransformerConfig::RSB);
    commRsb = TransformCommRsb::Ptr(new TransformCommRsb("bridge"));
    commRsb->init(configRsb);

    TransformCommunicator::Ptr commRos;
    TransformerConfig configRos;
    configRos.setCommType(TransformerConfig::ROS);
    commRos = TransformCommRos::Ptr(new TransformCommRos("bridge", configRos.getCacheTime(), false, 100));
    commRos->init(configRos);

    CommunicatorConnector<> rsbToRos(commRsb, commRos, "RsbToRos");
    CommunicatorConnector<> rosToRsb(commRos, commRsb, "RosToRsb");

    cout << "Communicators set up..." << endl;

    cout << "Stop with Ctrl + C!" << endl;
    Signal s = waitForSignal();

    cout << "Stop requested!" << endl;

    rsbToRos.stop();
    rosToRsb.stop();

    commRos->shutdown();
    commRsb->shutdown();

    cout << "Stopped!" << endl;

    ros::shutdown();

    return suggestedExitCode(s);
}
