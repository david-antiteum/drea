cmake_minimum_required( VERSION 3.12...3.16 )

project( drea-test )

find_package(Catch2 REQUIRED)

set( SOURCES 
	main.cpp
)
add_executable( ${PROJECT_NAME}
	${SOURCES}
)

set_target_properties( ${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" 
)

if( BUILD_SHARED_LIBS )
	add_compile_definitions(DREA_CORE_DLL)
endif()

target_link_libraries( ${PROJECT_NAME} 
	PRIVATE
		Catch2::Catch2
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
		PRIVATE
			Threads::Threads
	)
endif()

target_include_directories( ${PROJECT_NAME}
	PUBLIC
		../include ../include/thirdparty
		${CMAKE_CURRENT_BINARY_DIR}	
)

clang_format_setup( ${SOURCES} )

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

add_test(
	NAME ${PROJECT_NAME}
	COMMAND ${PROJECT_NAME}
)
