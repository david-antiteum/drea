cmake_minimum_required( VERSION 3.12 )
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project( calculator )
add_executable( ${PROJECT_NAME}
	main.cpp
	${CMAKE_CURRENT_BINARY_DIR}/commands.yml.h
	${CMAKE_CURRENT_BINARY_DIR}/version.yml.h
)
set_target_properties( ${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" 
)

if( BUILD_SHARED_LIBS )
	add_compile_definitions(DREA_CORE_DLL)
endif()

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )

include_directories( ../../include ../../include/thirdparty ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries( ${PROJECT_NAME} 
	drea
)

if( LINUX )
	set( CMAKE_SKIP_BUILD_RPATH FALSE )
	set( CMAKE_BUILD_WITH_INSTALL_RPATH TRUE )
	set_target_properties( ${PROJECT_NAME} PROPERTIES
		BUILD_RPATH "$ORIGIN"
		INSTALL_RPATH "$ORIGIN"
	)
endif()

if( UNIX )
	find_package( Threads REQUIRED )
	target_link_libraries( ${PROJECT_NAME} 
		Threads::Threads
	)
endif()

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
endif()

generate_cpp_resource_file( ${PROJECT_SOURCE_DIR} commands.yml ${CMAKE_CURRENT_BINARY_DIR}/commands.yml.h )
generate_cpp_resource_file( ${PROJECT_SOURCE_DIR} version.yml ${CMAKE_CURRENT_BINARY_DIR}/version.yml.h )
