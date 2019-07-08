###################### libfunccnt.so #######################
### library that needed to be inserted into the mutatee ####
set(FUNCCNT_SRC funccnt.cc)

add_library(funccnt SHARED ${FUNCCNT_SRC})
