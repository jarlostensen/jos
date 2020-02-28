
file(GLOB KERNEL_C_SRCS kernel/*.c kernel/arch/i386/*.c)
file(GLOB KERNEL_ASM_SRCS kernel/*.asm kernel/arch/i386/*.asm)

add_executable(${CMAKE_SOURCE_DIR}/jos.bin ${KERNEL_C_SRCS} ${KERNEL_ASM_SRCS})
set_target_properties(jos.bin PROPERTIES LINK_DEPENDS link_high.ld)
