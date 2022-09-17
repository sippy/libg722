cmake_minimum_required(VERSION 3.16)

# set the project name
project(arduino_libg22)

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRC_LIST_C CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/*.c" )

# define libraries
add_library (arduino_libg22 ${SRC_LIST_C})

target_compile_options(arduino_libg22 PRIVATE -Wdouble-promotion)
target_compile_definitions(arduino_libg22 PUBLIC -DARDUINO )


# define location for header files
target_include_directories(arduino_libg22 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
