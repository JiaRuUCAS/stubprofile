###################### libfunccnt.so #######################
### library that needed to be inserted into the mutatee ####
set(FUNCCNT_SRC funccnt.cc)

add_library(funccnt SHARED ${FUNCCNT_SRC})

install(TARGETS funccnt
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL})
