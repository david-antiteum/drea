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

# Enable clang-tidy either setting the app or using the option
option( ENABLE_CLANG_TIDY "Enable clang-tidy building." OFF )
if( ENABLE_CLANG_TIDY OR CLANG_TIDY )
	if( NOT CLANG_TIDY )
		if( WIN32 )
			find_program( CLANG_TIDY NAMES "clang-tidy" HINTS $ENV{VCINSTALLDIR}/Tools/Llvm/bin )
		else()
			find_program( CLANG_TIDY NAMES "clang-tidy" )
		endif()
	endif()
	if( CLANG_TIDY )
		set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY}" -checks=readability-*,cppcoreguidelines-*)
	endif()
endif()

add_subdirectory( core )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/hello )
	add_subdirectory( examples/saythis )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
endif()
