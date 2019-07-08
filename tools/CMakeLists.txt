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

######################### tracer ############################
# set source files
set(TRACER_SRC tracer.cc
			   funcmap.cc
			   funcmaptest.cc
			   test.cc)

# add build target
add_executable(stubprofile ${TRACER_SRC})
target_link_libraries(stubprofile ${BOOST_LIBS} dyninstAPI)
