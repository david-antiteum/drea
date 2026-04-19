cmake_minimum_required( VERSION 3.21 )

if( CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|Release|RelWithDebInfo|MinSizeRel)$")
	message( FATAL_ERROR "Invalid value for CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}. Use Debug, Release, RelWithDebInfo or MinSizeRel." )
endif()

if( DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE )
	message( "Using vcpkg settings: $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" )
	set( CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "" )
endif()

set( CMAKE_CXX_STANDARD 17 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )
set( CMAKE_CXX_EXTENSIONS OFF )

if( APPLE )
	set( CMAKE_OSX_DEPLOYMENT_TARGET 10.15 )
endif()

if( UNIX )
	set( CMAKE_SKIP_BUILD_RPATH FALSE )
	set( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )
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
