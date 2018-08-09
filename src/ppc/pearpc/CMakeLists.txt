set(PEARPC_SOURCES
    cpu/cpu_generic/ppc_alu.cpp
    cpu/cpu_generic/ppc_cpu.cpp
    cpu/cpu_generic/ppc_dec.cpp
    cpu/cpu_generic/ppc_exc.cpp
    cpu/cpu_generic/ppc_fpu.cpp
    cpu/cpu_generic/ppc_mmu.cpp
    cpu/cpu_generic/ppc_opc.cpp
    cpu/cpu_generic/ppc_vec.cpp
    uaeglue.cpp
)

add_library(pearpc OBJECT ${PEARPC_SOURCES})
