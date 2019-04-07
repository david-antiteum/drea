cmake_minimum_required( VERSION 3.12 )
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

option( BUILD_EXAMPLES "Build examples" ON )

#if( CMAKE_BUILD_TYPE MATCHES DEBUG )
	link_directories( "$ENV{VCPKG_ROOT}/installed/x64-windows/debug/lib" )
#else()
#	link_directories( "$ENV{VCPKG_ROOT}/installed/x64-windows/release/lib" )
#endif()

project( Drea )
add_subdirectory( core )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
endif()
