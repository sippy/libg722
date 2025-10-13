cmake_minimum_required(VERSION 3.16)

# set the project name
project(libg722 C)

# iOS-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(ENABLE_SHARED_LIB OFF CACHE BOOL "Build shared library" FORCE)
  set(ENABLE_STATIC_LIB ON CACHE BOOL "Build static library" FORCE)
  set(BUILD_TESTING OFF CACHE BOOL "Build tests" FORCE)
  message(STATUS "iOS build detected - using static library only")
endif()

option(ENABLE_SHARED_LIB "Build shared library" ON)
option(ENABLE_STATIC_LIB "Build static library" ON)

# lots of warnings and all warnings as errors
## add_compile_options(-Wall -Wextra )
set(CMAKE_C_STANDARD 11)

set(SRC_LIST_C g722_decode.c g722_encode.c)

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
  if( CMAKE_C_COMPILER_ID STREQUAL "GNU" )
    target_compile_options(g722_static PRIVATE -ffat-lto-objects)
  endif()
endif()

# macOS-specific settings
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  # Set a more appropriate default installation prefix for macOS
  # Users can still override this with -DCMAKE_INSTALL_PREFIX=...
  if(CMAKE_INSTALL_PREFIX STREQUAL "/usr/local")
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/Library/libg722" CACHE PATH "Installation prefix" FORCE)
    message(STATUS "macOS detected - setting default install prefix to: ${CMAKE_INSTALL_PREFIX}")
    message(STATUS "Override with: -DCMAKE_INSTALL_PREFIX=<path>")
  endif()

  # Disable bitcode for macOS builds
  add_compile_options(-fembed-bitcode=off)
  message(STATUS "Bitcode disabled for macOS builds")
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT lto_error)
if( lto_supported AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS" AND NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
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
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(STATUS "LTO disabled for iOS builds")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "LTO disabled for macOS builds")
  else()
    message(STATUS "LTO not supported: <${lto_error}>")
  endif()
endif()
