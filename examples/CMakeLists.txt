cmake_minimum_required(VERSION 2.6)

ADD_EXECUTABLE(pose-publisher PosePublisher.cpp )
TARGET_LINK_LIBRARIES(pose-publisher ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES})
