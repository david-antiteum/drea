cmake_minimum_required( VERSION 3.10 )

project( say )
add_executable( ${PROJECT_NAME} main.cpp )
set_target_properties( ${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" 
)

add_compile_definitions(DREA_CORE_DLL)

set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )

include_directories( ../../include ../../include/thirdparty )

target_link_libraries( ${PROJECT_NAME} 
	DreaCore
)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
endif()

if(WIN32)
	set(CMAKE_CXX_FLAGS_DEBUG "/MD")
endif()
