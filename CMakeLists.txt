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

function( generate_cpp_resource_file TARGET_NAME INPUT_DIR INPUT_FILE CPP_FILE )
	add_custom_command(OUTPUT ${CPP_FILE} COMMAND cd ${INPUT_DIR} && xxd -i ${INPUT_FILE} ${CPP_FILE} DEPENDS ${INPUT_DIR}/${INPUT_FILE})
	add_custom_target(${TARGET_NAME} ALL DEPENDS ${CPP_FILE})
endfunction()

add_subdirectory( core )

add_subdirectory( tools/unix/shell )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/hello )
	add_subdirectory( examples/say_this )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
endif()
