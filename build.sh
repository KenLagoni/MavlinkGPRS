rm MavlinkGPRSServer
g++ -Ilibraries/ -o MavlinkGPRSServer src/main.cpp src/connection.cpp src/parameters.cpp src/device.cpp src/deviceClient.cpp src/waypoints.cpp src/deviceServer.cpp src/mavPackageFilter.cpp -Wno-address-of-packed-member

