/*
 * PosePublisher.cpp
 *
 *  Created on: Mar 12, 2015
 *      Author: lziegler
 */


#include <stdlib.h>

#define BOOST_SIGNALS_NO_DEPRECATION_WARNING
#include <rsb/Informer.h>
#include <rsb/Factory.h>
#include <rsb/converter/ProtocolBufferConverter.h>
#include <rsb/converter/Repository.h>

#include <rst/geometry/Pose.pb.h>

using namespace std;
using namespace rsb;
using namespace rsb::converter;
using namespace rst::geometry;

int main(void) {

	Converter<string>::Ptr converter(new ProtocolBufferConverter<Pose>());
	converterRepository<string>()->registerConverter(converter);


    Factory& factory = getFactory();

    Informer<Pose>::Ptr informer = factory.createInformer<Pose> ("/nav/slampose");

    while (true) {
		Informer<Pose>::DataPtr s(new Pose());
		s->mutable_translation()->set_x(2.0);
		s->mutable_translation()->set_y(1.0);
		s->mutable_translation()->set_z(0.0);

		s->mutable_rotation()->set_qw(1.0);
		s->mutable_rotation()->set_qx(0.0);
		s->mutable_rotation()->set_qy(0.0);
		s->mutable_rotation()->set_qz(0.0);

		// Send the data.
		informer->publish(s);

		usleep(100*1000);
    }

    return EXIT_SUCCESS;
}

