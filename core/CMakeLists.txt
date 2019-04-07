cmake_minimum_required( VERSION 3.12 )
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project( DreaCore VERSION 0.0.1 DESCRIPTION "Drea Framework Core" LANGUAGES CXX )

find_package( yaml-cpp CONFIG REQUIRED )
find_package( nlohmann_json REQUIRED )
find_package( Boost REQUIRED )

link_libraries( ${YAML_CPP_LIBRARIES} nlohmann_json::nlohmann_json )

find_package( cpprestsdk )
if( cpprestsdk_FOUND )
	add_compile_definitions( CPPRESTSDK_ENABLED )
	link_libraries( cpprestsdk::cpprest)
endif()

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

if( WIN32 )
	add_compile_definitions( YAML_CPP_DLL )
endif()

if (MSVC)
	set_source_files_properties( Config.cpp PROPERTIES COMPILE_FLAGS /bigobj )
endif()

add_compile_definitions( _SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING )
add_compile_definitions( DREA_CORE_DLL )
add_compile_definitions( DREA_CORE_EXPORTS )

include( CheckCXXSymbolExists )
set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )
CHECK_CXX_SYMBOL_EXISTS( std::filesystem::path::preferred_separator filesystem cxx17fs )
if( cxx17fs )
	add_compile_definitions( CXX17FS )
endif()

include_directories( ../include/drea/core ../include/thirdparty ${YAML_CPP_INCLUDE_DIR} )

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
