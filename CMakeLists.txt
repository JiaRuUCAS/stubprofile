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

# add include path for DyninstAPI
include_directories(${DYNINST_BUILD_PATH}/include)

# add libraries searching path for DyninstAPI
link_directories(${DYNINST_BUILD_PATH}/lib)

####################### Boost ###########################
# Set list of Boost libraries
set(BOOST_LIBS boost_atomic
				boost_chrono
				boost_date_time
				boost_filesystem
				boost_system
				boost_thread
				boost_timer)

# set c++11 flags for gcc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# set c flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")

add_subdirectory(libfunccnt)
add_subdirectory(libprofile)

######################### tracer ############################
# set source files
#set(TRACER_SRC tracer.cc
#			   funcmap.cc
#			   funcmaptest.cc
#			   test.cc)

# add build target
#add_executable(dyninst-test ${TRACER_SRC})
#target_link_libraries(dyninst-test ${BOOST_LIBS} dyninstAPI)

message("Please set environment variables as follows:")
message("DYNINSTAPI_RT_LIB=${DYNINST_BUILD_PATH}/lib/libdyninstAPI_RT.so")
