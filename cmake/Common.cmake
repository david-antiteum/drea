cmake_minimum_required( VERSION 3.12...3.16 )

if( CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release)$")
	message( FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}. Use Debug or Release." )
endif()

if( EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake" )
	message( "Using conan settings: ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake" )
	include( ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake )
	conan_basic_setup()
elseif( DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE )
	message( "Using vcpkg settings: $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" )
	set( CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" )
endif()

set( CMAKE_CXX_STANDARD 17 )
if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
endif()

add_compile_definitions( _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING )
add_compile_definitions( _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING )

if( WIN32 )
	add_compile_definitions( _WIN32_WINNT=0x0A00 )
	add_compile_definitions( UNICODE )
	add_compile_definitions( _UNICODE )
endif()

if( WIN32 )
	add_compile_options( /W4 /MP /permissive- /wd4251 /wd4275 )
else()
	add_compile_options( -Wall -Wextra -pedantic )
endif()

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
