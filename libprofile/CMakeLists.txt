###################### libprofile.so #######################
### library that needed to be inserted into the mutatee ####
set(PROFILE_SRC array.c
				evlist.c
				evsel.c
				pmu.c
				profile.c
				threadmap.c
				util.c)

add_library(profile SHARED ${PROFILE_SRC})

install(TARGETS profile
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL})
