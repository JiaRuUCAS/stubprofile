###################### libfunccnt.so #######################
### library that needed to be inserted into the mutatee ####
set(PROBE_SRC
		funccnt.c
		thread.c
		util.c)

add_library(probe SHARED ${PROBE_SRC})

install(TARGETS probe
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL})
