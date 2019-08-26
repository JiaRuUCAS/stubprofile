cmake_minimum_required (VERSION 3.4.0)
project (stubprofile)

set(CMALE_CXX_STANDARD 11)

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set installation path
set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR}/build)
set(LIB_INSTALL ${CMAKE_INSTALL_PREFIX}/lib) 
set(HEADER_INSTALL ${CMAKE_INSTALL_PREFIX}/include)  

#################### DyninstAPI #########################
# set DyninstAPI path
set(DYNINST_PATH ${PROJECT_SOURCE_DIR}/dyninst)
set(DYNINST_BUILD_PATH ${CMAKE_INSTALL_PREFIX}/dyninst)

# set Dyninst-tiny path
set(DYNINST_TINY_PATH ${PROJECT_SOURCE_DIR}/dyninstT)
set(DYNINST_TINY_BUILD_PATH ${CMAKE_INSTALL_PREFIX}/dyninstT)

#add_subdirectory(dyninst-tiny)

set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" "${PROJECT_SOURCE_DIR}/cmake/Modules" ${CMAKE_MODULE_PATH})
include(${PROJECT_SOURCE_DIR}/cmake/shared.cmake)
include(ElfUtils)

add_subdirectory(dyninstT)

#execute_process(COMMAND sh ${PROJECT_SOURCE_DIR}/setup_dyninst.sh
#		${DYNINST_PATH} ${DYNINST_BUILD_PATH})

# set c flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Wall -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")

add_subdirectory(libfunccnt)
add_subdirectory(libprofile)

add_subdirectory(tools)

message("Please set environment variables as follows:")
message("DYNINSTAPI_RT_LIB=${DYNINST_BUILD_PATH}/lib/libdyninstAPI_RT.so")
message("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${DYNINST_BUILD_PATH}/lib")
