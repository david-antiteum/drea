cmake_minimum_required( VERSION 3.10 )

project( DreaCore VERSION 0.0.1 DESCRIPTION "Drea Framework Core" LANGUAGES CXX )

find_package( yaml-cpp REQUIRED )

add_library( ${PROJECT_NAME} SHARED 
	App.cpp
	Config.cpp
	Commander.cpp
	Option.cpp
)
set_target_properties( ${PROJECT_NAME}
	PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" 
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
	LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib" 
	VERSION ${PROJECT_VERSION}
	SOVERSION 0
)

target_link_libraries( ${PROJECT_NAME} ${YAML_CPP_LIBRARIES} )

add_compile_definitions( DREA_CORE_DLL )
add_compile_definitions( DREA_CORE_EXPORTS )

include( CheckCXXSymbolExists )
set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )
CHECK_CXX_SYMBOL_EXISTS( std::filesystem::path::preferred_separator filesystem cxx17fs )
if( cxx17fs )
	add_compile_definitions( CXX17FS )
endif()

include_directories( ../include/core ../include/thirdparty ${YAML_CPP_INCLUDE_DIR} )

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Config
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY ../include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ${PROJECT_NAME}Config DESTINATION share/${PROJECT_NAME}/cmake)

export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_NAME}Config.cmake)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14)
endif()

if(WIN32)
	set(CMAKE_CXX_FLAGS_DEBUG "/MD")
endif()
