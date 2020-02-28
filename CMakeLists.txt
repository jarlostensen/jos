#see for example https://gitlab.com/CLIUtils/modern-cmake/-/tree/master/examples/extended-project

cmake_minimum_required(VERSION 3.7 FATAL_ERROR)

project(jOSKernel 
    VERSION 0.1 
    DESCRIPTION "The simple jOS kernel"
    LANGUAGES ASM C)

# this is global so we can set it here in the root 
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wall,-T${CMAKE_SOURCE_DIR}/kernel/link_high.ld")

add_subdirectory(libc)
add_subdirectory(kernel)
