cmake_minimum_required( VERSION 3.12 )
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project( Drea )
add_subdirectory( core )
add_subdirectory( examples/say )
add_subdirectory( examples/calculator )
