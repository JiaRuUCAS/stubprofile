include(${PROJECT_SOURCE_DIR}/cmake/CheckCXX11Features.cmake)
if(NOT HAS_CXX11_AUTO)
		message(FATAL_ERROR "No support for C++11 auto found. Stubprofile requires this compiler feature.")
else()
  message(STATUS "C++11 support found, required flags are: ${CXX11_COMPILER_FLAGS}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_COMPILER_FLAGS}")
