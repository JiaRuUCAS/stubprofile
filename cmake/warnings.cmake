set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -W -Wall -Wpointer-arith -Wcast-qual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wpointer-arith -Wcast-qual -Woverloaded-virtual")

if (CMAKE_C_COMPILER_ID MATCHES GNU)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align -Wno-non-template-friend -Wno-unused-local-typedefs -Wno-deprecated-declarations")
endif()
