cmake_minimum_required(VERSION 3.16)

# set the project name
project(libg722 C)

option(ENABLE_SHARED_LIB "Build shared library" ON)
option(ENABLE_STATIC_LIB "Build static library" ON)

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_C_STANDARD 11)

file(GLOB_RECURSE SRC_LIST_C CONFIGURE_DEPENDS  "${PROJECT_SOURCE_DIR}/g722*.c" )

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()

# define libraries
if( ENABLE_SHARED_LIB )
  add_library (g722 SHARED ${SRC_LIST_C})
  set_target_properties(g722 PROPERTIES VERSION 0)
  set_target_properties(g722 PROPERTIES PUBLIC_HEADER "g722_codec.h;g722_decoder.h;g722_encoder.h;g722.h")
  add_executable(test_dynamic test.c)
  target_link_libraries(test_dynamic g722)
  add_test(NAME TestDynamic COMMAND ${PROJECT_SOURCE_DIR}/scripts/do-test.sh $<TARGET_FILE:test_dynamic>)
  install(TARGETS g722 LIBRARY DESTINATION lib PUBLIC_HEADER DESTINATION include)
  target_compile_options(g722 PRIVATE -Wdouble-promotion -Wno-attributes)
  target_include_directories(g722 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()
if( ENABLE_STATIC_LIB )
  add_library (g722_static STATIC ${SRC_LIST_C})
  set_target_properties(g722_static PROPERTIES PUBLIC_HEADER "g722_codec.h;g722_decoder.h;g722_encoder.h;g722.h")
  set_target_properties(g722_static PROPERTIES OUTPUT_NAME g722)
  add_executable(test_static test.c)
  target_link_libraries(test_static g722_static)
  add_test(NAME TestStatic COMMAND ${PROJECT_SOURCE_DIR}/scripts/do-test.sh $<TARGET_FILE:test_static>)
  install(TARGETS g722_static ARCHIVE DESTINATION lib PUBLIC_HEADER DESTINATION include)
  target_compile_options(g722_static PRIVATE -Wdouble-promotion -Wno-attributes)
  target_include_directories(g722_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
if( lto_supported )
  message(STATUS "LTO enabled")
  if( ENABLE_SHARED_LIB )
    set_property(TARGET g722 PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET test_dynamic PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
  if( ENABLE_STATIC_LIB )
    set_property(TARGET g722_static PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    set_property(TARGET test_static PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
  endif()
else()
    message(STATUS "LTO not supported: <${lto_error}>")
endif()
