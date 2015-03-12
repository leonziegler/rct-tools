cmake_minimum_required(VERSION 2.6)
            
# --- generate executable
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/core/src ${CMAKE_SOURCE_DIR}/rsb/src ${CMAKE_SOURCE_DIR}/ros/src ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/../../core/src)

ADD_EXECUTABLE(rct-static-publisher rct/RctStaticPublisher.cpp rct/parsers/ParserXML.cpp rct/parsers/ParserINI.cpp)
TARGET_LINK_LIBRARIES(rct-static-publisher ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES})
SET_TARGET_PROPERTIES(rct-static-publisher PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})

IF(tf2_ros_FOUND)
    ADD_EXECUTABLE(rct-ros-bridge rct/RctRosBridge.cpp)
    TARGET_LINK_LIBRARIES(rct-ros-bridge ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES} ${tf2_ros_LIBRARIES})
    SET_TARGET_PROPERTIES(rct-ros-bridge PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})
ENDIF(tf2_ros_FOUND)

ADD_EXECUTABLE(rct-echo rct/RctEcho.cpp)
TARGET_LINK_LIBRARIES(rct-echo ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES} )
SET_TARGET_PROPERTIES(rct-echo PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})
                                 
ADD_EXECUTABLE(rct-from-rst rct/RctFromRst.cpp rct/parsers/ParserXML.cpp rct/parsers/ParserINI.cpp)
TARGET_LINK_LIBRARIES(rct-from-rst ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES} )
SET_TARGET_PROPERTIES(rct-from-rst PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})

SET(RCT-VIEW-SOURCES rct/RctView.cpp)
SET(RCT-VIEW-LIBS ${RCT_LIBRARIES} ${RSB_LIBRARIES} ${RST_LIBRARIES})
IF(POPPLERQT5_FOUND OR POPPLERQT4_FOUND)
    LIST(APPEND RCT-VIEW-SOURCES rct/viewer/imageviewer.cpp)
ENDIF(POPPLERQT5_FOUND OR POPPLERQT4_FOUND)
IF(POPPLERQT5_FOUND)
    LIST(APPEND RCT-VIEW-LIBS ${POPPLERQT5_LIBRARIES} ${Qt5Widgets_LIBRARIES})
ENDIF(POPPLERQT5_FOUND)
IF(POPPLERQT4_FOUND)
    LIST(APPEND RCT-VIEW-LIBS ${POPPLERQT4_LIBRARIES} ${QT_LIBRARIES})
ENDIF(POPPLERQT4_FOUND)
ADD_EXECUTABLE(rct-view ${RCT-VIEW-SOURCES})
TARGET_LINK_LIBRARIES(rct-view ${RCT-VIEW-LIBS})
SET_TARGET_PROPERTIES(rct-view PROPERTIES
                                 VERSION ${RCTTOOLS_VERSION}
                                 SOVERSION ${RCTTOOLS_VERSION_SHORT})

# --- install target
INSTALL(TARGETS rct-static-publisher
        RUNTIME DESTINATION ${BINDIR})
