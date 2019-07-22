# The test suite needs this as a list rather than a bunch
# of definitions so that we can append _test to it. 

set (CAP_DEFINES
     -Dcap_dynamic_heap 
     -Dcap_liveness 
     -Dcap_threads
)

if(USE_GNU_DEMANGLER)
set (CAP_DEFINES ${CAP_DEFINES} -Dcap_gnu_demangler)
endif()

set (ARCH_DEFINES -Darch_x86_64 -Darch_64bit)
set (CAP_DEFINES ${CAP_DEFINES} 
             -Dcap_32_64
             -Dcap_fixpoint_gen 
             -Dcap_noaddr_gen
             -Dcap_registers
             -Dcap_stripped_binaries 
             -Dcap_tramp_liveness
             -Dcap_stack_mods
    )

set (OS_DEFINES -Dos_linux)
set (CAP_DEFINES ${CAP_DEFINES} 
             -Dcap_async_events
             -Dcap_binary_rewriter
             -Dcap_dwarf
             -Dcap_mutatee_traps
             -Dcap_ptrace
    )
set (BUG_DEFINES -Dbug_syscall_changepc_rewind -Dbug_force_terminate_failure)

set (OLD_DEFINES -Dx86_64_unknown_linux2_4)

set (UNIFIED_DEFINES ${CAP_DEFINES} ${BUG_DEFINES} ${ARCH_DEFINES} ${OS_DEFINES} ${OLD_DEFINES})

foreach (def ${UNIFIED_DEFINES})
  add_definitions (${def})
endforeach()


set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${UNIFIED_DEF_STRING}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${UNIFIED_DEF_STRING}")

message(STATUS "Set arch and platform based definitions")

