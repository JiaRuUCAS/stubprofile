set(DYNT_COMMON_SRC
		dyn_regs.C
		util.C
		)

add_library(dyninstT_common STATIC ${DYNT_COMMON_SRC})

target_link_private_libraries(dyninstT_common ${BOOST_LIBS})

install(TARGETS dyninstT_common
		RUNTIME DESTINATION ${DYNT_BIN_PATH}
		LIBRARY DESTINATION ${DYNT_LIB_PATH}
		ARCHIVE DESTINATION ${DYNT_LIB_PATH}
		)
