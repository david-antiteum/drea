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
	add_custom_command( 
		OUTPUT ${CPP_FILE}
		COMMAND cd ${INPUT_DIR} && xxd -i ${INPUT_FILE} ${CPP_FILE} 
		DEPENDS ${INPUT_DIR}/${INPUT_FILE}
		COMMENT "Generating header file ${CPP_FILE}"
		VERBATIM
	)
	set_source_files_properties( ${CPP_FILE} PROPERTIES GENERATED TRUE )
endfunction()

find_program( CLANG_TIDY NAMES "clang-tidy" )
if( CLANG_TIDY )
	set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}" -checks=readability-*,cppcoreguidelines-*)
endif()

add_subdirectory( core )

add_subdirectory( tools/unix/shell )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/hello )
	add_subdirectory( examples/saythis )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
endif()
