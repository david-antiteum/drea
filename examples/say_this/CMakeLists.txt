cmake_minimum_required( VERSION 3.12 )
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project( say_this )
add_executable( ${PROJECT_NAME} main.cpp )
set_target_properties( ${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" 
)

add_compile_definitions(DREA_CORE_DLL)

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )

include_directories( ../../include ../../include/thirdparty ${CMAKE_CURRENT_BINARY_DIR} )

target_link_libraries( ${PROJECT_NAME} 
	DreaCore
)

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
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
endif()

generate_cpp_resource_file( "${PROJECT_NAME}_CMD_H" ${PROJECT_SOURCE_DIR} commands.yml ${CMAKE_CURRENT_BINARY_DIR}/commands.yml.h )
