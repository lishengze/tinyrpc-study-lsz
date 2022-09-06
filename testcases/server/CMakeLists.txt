# copyright [2018] [wxblockchain]
# author: daniel.bian
# description: galaxy project main CMakeLists.txt
# update-date:
#   2019-01-04  init version
##############################################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(server)

SET(BIN_NAME server)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # Generate compile_commands.json

set(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -std=c++11 -pthread -Wall -Wno-deprecated -Wno-unused-but-set-variable")


## Set CPP SRC

ENABLE_LANGUAGE(ASM)

AUX_SOURCE_DIRECTORY(../../tinyrpc/comm COMM_SRC)
AUX_SOURCE_DIRECTORY(../../tinyrpc/coroutine COROUTINE_SRC)
AUX_SOURCE_DIRECTORY(../../tinyrpc/net NET_SRC)
AUX_SOURCE_DIRECTORY(../../tinyrpc/net/http HTTP_SRC)
AUX_SOURCE_DIRECTORY(../../tinyrpc/net/tcp TCP_SRC)
AUX_SOURCE_DIRECTORY(../../tinyrpc/net/tinypb TINYPB_SRC)
AUX_SOURCE_DIRECTORY(../../tinyrpc/tinyxml XML_SRC)
AUX_SOURCE_DIRECTORY(../pb PB_SRC)
AUX_SOURCE_DIRECTORY(./ TEST_SRC)

INCLUDE_DIRECTORIES(../../)
INCLUDE_DIRECTORIES(../../tinyrpc)
INCLUDE_DIRECTORIES(../pb)

SET(BIN_SRC ${COMM_SRC} ${COROUTINE_SRC} ${NET_SRC} ${HTTP_SRC} 
            ${TCP_SRC} ${TINYPB_SRC} ${XML_SRC} ${PB_SRC} ${TEST_SRC}
            ../../tinyrpc/coroutine/coctx_swap.S)

ADD_EXECUTABLE(${BIN_NAME} ${BIN_SRC})
target_link_libraries(${BIN_NAME} protobuf mysqlclient dl)


