if(ENABLE_LTO)
    set(LTO_FLAGS "-flto")
    set(LTO_LINK_FLAGS "-fuse-ld=gold")
else()
    set(LTO_FLAGS "")
    set(LTO_LINK_FLAGS "")
endif()
set (CMAKE_C_FLAGS_DEBUG "-O0 -g")
set (CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

set (CMAKE_C_FLAGS_RELEASE "-O2 ${LTO_FLAGS}")
set (CMAKE_CXX_FLAGS_RELEASE "-O2 ${LTO_FLAGS}")

set (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g ${LTO_FLAGS}")
set (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g ${LTO_FLAGS}")

set (FORCE_FRAME_POINTER "-fno-omit-frame-pointer")
# Ensure each library is fully linked
set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")

set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LTO_LINK_FLAGS}")
set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${LTO_LINK_FLAGS}")
message(STATUS "Set optimization flags")
