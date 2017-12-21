project(holetest)

cmake_minimum_required(VERSION 2.8)

SET(CMAKE_CXX_FLAGS "-Wall -std=c++14")

#SET(TOOLCHAIN_DIR "/home/czh/workspace/GM8136SDKreleasev2.0/Software/Embedded_Linux/source/arm-linux-3.3/toolchain_gnueabi-4.4.0_ARMv5TE/")
#SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}usr/bin/arm-unknown-linux-uclibcgnueabi-gcc)
#SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}usr/bin/arm-unknown-linux-uclibcgnueabi-g++)
#SET(LINUX_SRC ~/workspace/GM8136SDKreleasev2.0/Software/Embedded_Linux/source/arm-linux-3.3/linux-3.3-fa/)

#SET(CMAKE_C_COMPILER "gcc")
#SET(CMAKE_CXX_COMPILER "g++")

file(GLOB CLIENT_LIB_SRCS "./client/*.cpp")
file(GLOB SERVER_LIB_SRCS "./server/*.cpp")
file(GLOB COMMON_LIB_SRCS "./common/*.cpp")


#aux_source_directory(./src DIR_SRCS)

#link_directories("/usr/local/lib")

include_directories(./client ./server/ ./common/)

link_libraries(pthread)

add_executable(clientnodetest ${CLIENT_LIB_SRCS} ${COMMON_LIB_SRCS} clientnodemain.cpp)
add_executable(holetest ${CLIENT_LIB_SRCS} ${SERVER_LIB_SRCS} ${COMMON_LIB_SRCS} main.cpp)
add_executable(servertest ${SERVER_LIB_SRCS} ${COMMON_LIB_SRCS} servermain.cpp)
add_executable(servernodetest ${CLIENT_LIB_SRCS} ${COMMON_LIB_SRCS} servernodemain.cpp)
