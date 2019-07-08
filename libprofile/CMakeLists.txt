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
