include(Boost)

# Set list of Boost libraries
set(BOOST_LIBS boost_atomic
				boost_chrono
				boost_date_time
				boost_filesystem
				boost_system
				boost_thread
				boost_timer)

set(DYNT_LIB_PATH ${DYNINST_TINY_BUILD_PATH}/lib)
set(DYNT_BIN_PATH ${DYNINST_TINY_BUILD_PATH}/bin)

include_directories(
		${DYNINST_TINY_BUILD_PATH}/include
		${DYNINST_TINY_PATH})

add_subdirectory(common)
