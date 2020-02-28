SET(CMAKE_SYSTEM_NAME Generic)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_ASM_COMPILER nasm)
SET(CMAKE_C_COMPILER clang-11)
SET(CMAKE_ASM_FLAGS "-target i386-none-elf ${CMAKE_ASM_FLAGS}" CACHE STRING "" FORCE)
SET(CMAKE_C_FLAGS "-target i386-none-elf -fno-builtin -nostdinc -ffreestanding ${CMAKE_C_FLAGS}" CACHE STRING "" FORCE)
SET(CMAKE_EXE_LINKER_FLAGS_INIT "-target i386-linux-elf -nostdlib")
