# check kernel: we only support UNIX
if(NOT UNIX)
	message(FATAL_ERROR "Building stubprofile from source is not supported on this platform")
endif()

set (PLATFORM $ENV{PLATFORM})
message(STATUS "-- Input platform: ${PLATFORM}")
set (VALID_PLATFORMS
    x86_64-unknown-linux2.4
    )

if (NOT PLATFORM)
set (INVALID_PLATFORM true)
else()
list (FIND VALID_PLATFORMS ${PLATFORM} PLATFORM_FOUND)
  if (PLATFORM_FOUND EQUAL -1)
  set (INVALID_PLATFORM true)
  endif()
endif()


execute_process (COMMAND ${PROJECT_SOURCE_DIR}/scripts/sysname OUTPUT_VARIABLE SYSNAME_OUT)
string(REPLACE "\n" "" SYSPLATFORM ${SYSNAME_OUT})

if (INVALID_PLATFORM)
# Try to set it automatically
execute_process (COMMAND ${PROJECT_SOURCE_DIR}/scripts/dynsysname ${SYSNAME_OUT}
		OUTPUT_VARIABLE DYNSYSNAME_OUT
                 )
		 string (REPLACE "\n" "" PLATFORM ${DYNSYSNAME_OUT})
message (STATUS "-- Attempting to automatically identify platform: ${PLATFORM}")
endif()

list (FIND VALID_PLATFORMS ${PLATFORM} PLATFORM_FOUND)

if (PLATFORM_FOUND EQUAL -1)
message (FATAL_ERROR "Error: unknown platform ${PLATFORM}; please set the PLATFORM environment variable to one of the following options: ${VALID_PLATFORMS}")
endif()
message("platform ${PLATFORM}")
