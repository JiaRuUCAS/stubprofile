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
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11")

######################### tracer ############################
# set source files
set(TRACER_SRC
		count.cc
		edit.cc
		funcmap.cc
		funcmaptest.cc
		test.cc
		tracer.cc)

# add build target
add_executable(stubprofile ${TRACER_SRC})
target_link_libraries(stubprofile ${BOOST_LIBS} dyninstAPI)

set_target_properties(stubprofile PROPERTIES INSTALL_RPATH "${DYNINST_BUILD_PATH}/lib")

install(TARGETS stubprofile
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL})
