# rct-tools

Software tools for the Robotics Coordinate Transform library.

## CmdLine Tools

Sending static transforms

    $ rct-static-publisher -n "example-static-publisher" -c transforms.xml

A examplary configuration file:

    <rct>
        <core>
            <cachetime value="30" />
        </core>
        <transforms>
            <transform parent="foo" child="bar">
                <translation x="1.0" y="2.0" z="3.0" unit="METER" />
                <rotation qw="4.0" qx="3.0" qy="2.0" qz="1.0" unit="RADIAN" />
            </transform>
            <transform parent="bar" child="baz">
                <translation x="1.0" y="2.0" z="3.0" unit="CENTIMETER" />
                <rotation roll="4.0" pitch="3.0" yaw="2.0" unit="DEGREE" />
            </transform>
        </transforms>
    <rct>

Print a transform

    $ rct-echo "foo" "bar"

View the complete coordinate system tree

    $ rct-view

Convert rst Pose events to rct transforms

    $ rct-from-rst -c rst-rct-config.xml

A examplary configuration file:

    <rct>
        <core>
            <cachetime value="30" />
        </core>
        <messages>
            <message  parent="foo" child="bar" scope="/nav/slampose" authority="slam" />
            <message  parent="bar" child="baz" scope="/arm/ee/pose" authority="armserver" />
        </messages>
    </rct>

Create a bridge between ROS/TF and RSB/RCT

    $ rct-ros-bridge

