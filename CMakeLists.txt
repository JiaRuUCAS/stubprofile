cmake_minimum_required (VERSION 3.4.0)
project (stubprofile)

set(CMALE_CXX_STANDARD 11)

# set installation path
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build)

# set cmake searching path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")


#################### DyninstAPI #########################
# set DyninstAPI path
set(DYNINST_PATH ${PROJECT_SOURCE_DIR}/dyninst)
set(DYNINST_BUILD_PATH ${CMAKE_INSTALL_PREFIX}/dyninst)

#add_subdirectory(dyninst)

execute_process(COMMAND sh ${PROJECT_SOURCE_DIR}/setup_dyninst.sh
		${DYNINST_PATH} ${DYNINST_BUILD_PATH})

# set c flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")

add_subdirectory(libfunccnt)
add_subdirectory(libprofile)

add_subdirectory(tools)

message("Please set environment variables as follows:")
message("DYNINSTAPI_RT_LIB=${DYNINST_BUILD_PATH}/lib/libdyninstAPI_RT.so")
