set(DYNT_SYMTAB_SRC
		annotations.C
		exception.C
		Module.C
		Region.C
		relocation.C
		Symbol.C
		Symtab.C
		Type.C
		)

add_library(dyninstT_symtab SHARED ${DYNT_SYMTAB_SRC})

target_link_private_libraries(dyninstT_symtab
		${BOOST_LIBS} ${TBB_LIBS} dyninstT_common)

install(TARGETS dyninstT_symtab
		RUNTIME DESTINATION ${DYNT_BIN_PATH}
		LIBRARY DESTINATION ${DYNT_LIB_PATH}
		ARCHIVE DESTINATION ${DYNT_LIB_PATH}
		)
