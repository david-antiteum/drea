cmake_minimum_required( VERSION 3.21 )
project( multi )

add_executable( ${PROJECT_NAME}
	main.cpp
	${CMAKE_CURRENT_BINARY_DIR}/commands.yml.h
)
set_target_properties( ${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
drea_set_rpath( ${PROJECT_NAME} )

target_include_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} )
target_link_libraries( ${PROJECT_NAME} PRIVATE drea )

include( GNUInstallDirs )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

generate_cpp_resource_file( ${PROJECT_SOURCE_DIR} commands.yml ${CMAKE_CURRENT_BINARY_DIR}/commands.yml.h )
