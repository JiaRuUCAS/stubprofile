set(DYNT_COMMON_SRC
		addrtranslate.C
		addrtranslate-auxv.C
		addrtranslate-linux.C
		addrtranslate-sysv.C
		Annotatable.C
		debug_common.C
		dthread.C
		dyn_regs.C
		fraction.C
		linuxKludges.C
		lprintf.C
		MappedFile.C
		mcs-lock.C
		parseauxv.C
		pathName.C
		pfq-rwlock.C
		race-detector-annotations.C
		serialize-bin.C
		serialize-xml.C
		sha1.C
		stats.C
		string-regex.C
		Time.C
		timing.C
		timing-linux.C
		Timer.C
		Types.C
		util.C
		VariableLocation.C
		)

add_library(dyninstT_common ${DYNT_COMMON_SRC})

target_link_private_libraries(dyninstT_common pthread)
target_link_private_libraries(dyninstT_common ${BOOST_LIBS})

install(TARGETS dyninstT_common
		RUNTIME DESTINATION ${DYNT_BIN_PATH}
		LIBRARY DESTINATION ${DYNT_LIB_PATH}
		ARCHIVE DESTINATION ${DYNT_LIB_PATH}
		)
