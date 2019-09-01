set(DYNT_SYMTAB_SRC
		Aggregate.C
		annotations.C
		Collections.C
		debug.C
		exception.C
		Function.C
		Module.C
		Object.C
		Object-elf.C
		parseStab.C
		Region.C
		Variable.C
		relocation.C
		Symbol.C
		Symtab.C
		Symtab-edit.C
		Symtab-lookup.C
		Type.C
		)

add_library(dyninstT_symtab SHARED ${DYNT_SYMTAB_SRC})

target_link_private_libraries(dyninstT_symtab
		elf ${BOOST_LIBS} ${TBB_LIBS} dyninstT_common)

install(TARGETS dyninstT_symtab
		RUNTIME DESTINATION ${DYNT_BIN_PATH}
		LIBRARY DESTINATION ${DYNT_LIB_PATH}
		ARCHIVE DESTINATION ${DYNT_LIB_PATH}
		)
