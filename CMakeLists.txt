cmake_minimum_required( VERSION 3.12...3.16 )

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include( Common )
include( Tools )

project( Drea VERSION 0.10.5 DESCRIPTION "Drea Framework" LANGUAGES CXX )

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include( CTest )
endif()

option( BUILD_EXAMPLES "Build examples" ON )

sonarqube_setup()

add_subdirectory( core )

if( BUILD_EXAMPLES )
	add_subdirectory( examples/hello )
	add_subdirectory( examples/saythis )
	add_subdirectory( examples/say )
	add_subdirectory( examples/calculator )
	add_subdirectory( examples/multi )
endif()
if( BUILD_TESTING )
	add_subdirectory( tests )
endif()
