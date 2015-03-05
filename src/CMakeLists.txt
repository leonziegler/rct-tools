cmake_minimum_required(VERSION 2.6)
            
# --- generate executable
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core/src ${CMAKE_SOURCE_DIR}/rsb/src ${CMAKE_SOURCE_DIR}/ros/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../../core/src)

ADD_EXECUTABLE(rct-static-publisher rct/RctStaticPublisher.cpp rct/parsers/ParserXML.cpp rct/parsers/ParserINI.cpp)
TARGET_LINK_LIBRARIES(rct-static-publisher ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES} ${Log4cxx_LIBRARIES})
SET_TARGET_PROPERTIES(rct-static-publisher PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})

ADD_EXECUTABLE(rct-echo rct/RctEcho.cpp)
TARGET_LINK_LIBRARIES(rct-echo ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES} ${Log4cxx_LIBRARIES})
SET_TARGET_PROPERTIES(rct-echo PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})
                                 
ADD_EXECUTABLE(rct-view rct/RctView.cpp)
TARGET_LINK_LIBRARIES(rct-view ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES} ${Log4cxx_LIBRARIES})
SET_TARGET_PROPERTIES(rct-view PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})

# --- install target
INSTALL(TARGETS rct-static-publisher
        RUNTIME DESTINATION ${BINDIR})
