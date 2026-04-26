cmake_minimum_required( VERSION 3.21 )

list( APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

include( Common )
include( Tools )

project( drea VERSION 0.31.0 DESCRIPTION "Drea Framework" LANGUAGES CXX )

if( CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME )
	include( CTest )
endif()

option( BUILD_EXAMPLES  "Build examples"                                               ON  )
option( BUILD_REST_USE  "Build support to access REST services"                        ON  )
option( ENABLE_AWS      "Build support for AWS Secrets Manager as a config source"     OFF )
option( USE_SYSTEM_LIBS "Use system-installed dependencies (apt/brew/etc) not vcpkg"   OFF )

sonarqube_setup()

add_subdirectory( core )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/hello )
	add_subdirectory( examples/saythis )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
	add_subdirectory( examples/multi )
	add_subdirectory( examples/hidden )
	add_subdirectory( examples/groups )
	if( ENABLE_AWS )
		add_subdirectory( examples/aws-secrets )
	endif()
endif()
if( BUILD_TESTING )
	add_subdirectory( tests )
endif()
