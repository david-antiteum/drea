cmake_minimum_required( VERSION 3.21 )
project( drea-test )

find_package( Catch2 REQUIRED )

set( SOURCES
	main.cpp
	CommandTest.cpp
	OptionTest.cpp
	CommanderTest.cpp
	ConfigTest.cpp
	GroupsTest.cpp
	LoggerTest.cpp
)

add_executable( ${PROJECT_NAME} ${SOURCES} )

set_target_properties( ${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
drea_set_rpath( ${PROJECT_NAME} )

# core/ so tests can exercise internal pieces (json_lines_formatter)
target_include_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../core )
target_link_libraries( ${PROJECT_NAME}
	PRIVATE
		Catch2::Catch2WithMain
		drea
)

clang_format_setup( ${SOURCES} )

include( GNUInstallDirs )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

add_test(
	NAME ${PROJECT_NAME}
	COMMAND ${PROJECT_NAME}
)
