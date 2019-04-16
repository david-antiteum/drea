cmake_minimum_required( VERSION 3.12 )

if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set( USING_VCPKG "ON" )
	set( CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" )
	message( "Using vcpkg settings" )
endif()

project( Drea VERSION 0.0.1 DESCRIPTION "Drea Framework" LANGUAGES CXX )

if( NOT DEFINED USING_VCPKG AND EXISTS "${CMAKE_BINARY_DIR}/conanbuildinfo.cmake")
	message( "Using conan settings" )
	include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
	conan_basic_setup()
endif()

set( CMAKE_CXX_STANDARD 17 )

option( BUILD_EXAMPLES "Build examples" ON )

if( CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Release" )
	link_directories( "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/lib" )
else()
	link_directories( "$ENV{VCPKG_ROOT}/installed/$ENV{VCPKG_DEFAULT_TRIPLET}/debug/lib" )
endif()

add_subdirectory( core )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/hello )
	add_subdirectory( examples/say_this )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
endif()
