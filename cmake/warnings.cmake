set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wpointer-arith -Wcast-qual -Woverloaded-virtual")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Waddress -Warray-bounds -Wchar-subscripts -Wcomment")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wformat -Wmissing-braces -Wnonnull -Wreturn-type -Wsign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsequence-point -Wstrict-aliasing -Wstrict-overflow=1 -Wswitch -Wtrigraphs")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wuninitialized -Wunknown-pragmas -Wunused-function -Wunused-label -Wvolatile-register-var")

if (CMAKE_C_COMPILER_ID MATCHES GNU)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wcast-align")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-align -Wno-non-template-friend -Wno-unused-local-typedefs -Wno-deprecated-declarations")
endif()
