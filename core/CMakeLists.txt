cmake_minimum_required( VERSION 3.12 )
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

project( drea VERSION 0.11.1 DESCRIPTION "Drea Framework" LANGUAGES CXX )

option( BUILD_REST_USE "Build support to access REST services" ON )

find_package( fmt CONFIG REQUIRED )
find_package( spdlog CONFIG REQUIRED )
find_package( yaml-cpp CONFIG REQUIRED )
find_package( Microsoft.GSL CONFIG )
find_package( tl-expected CONFIG REQUIRED )

link_libraries( yaml-cpp fmt::fmt spdlog::spdlog tl::expected )

if( BUILD_REST_USE )
	find_package( uriparser CONFIG REQUIRED )
	find_package( nlohmann_json CONFIG REQUIRED )
	find_package( Boost REQUIRED )
	find_package( consulcpp CONFIG REQUIRED )
	add_compile_definitions( ENABLE_REST_USE )
else()
	message( "Disabling REST support." )
	find_package( nlohmann_json CONFIG )
endif()

if( consulcpp_FOUND )
	link_libraries( consulcpp )
endif()
if( uriparser_FOUND )
	link_libraries( uriparser::uriparser )
endif()
if( nlohmann_json_FOUND )
	link_libraries( nlohmann_json::nlohmann_json )
	add_compile_definitions( ENABLE_JSON )
else()
	message( "Could not find nlohmann_json. Disabling JSON support." )
endif()

find_path( CPPTOML_INCLUDE_DIR cpptoml.h )
if(CPPTOML_INCLUDE_DIR STREQUAL "CPPTOML_INCLUDE_DIR-NOTFOUND")
	message( "Could not find cpptoml. Disabling TOML support." )
else()
	add_compile_definitions( ENABLE_TOML )
	include_directories(${CPPTOML_INCLUDE_DIR})
endif()

add_library( ${PROJECT_NAME} 
	App.cpp
	Config.cpp
	Commander.cpp
	Option.cpp
	Command.cpp
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

if( BUILD_SHARED_LIBS )
	add_compile_definitions( DREA_CORE_DLL )
	add_compile_definitions( DREA_CORE_EXPORTS )
	message( "Drea Dynamic Build" )
else()
	message( "Drea Static Build" )
endif()

include( CheckCXXSymbolExists )
set_property( TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17 )
CHECK_CXX_SYMBOL_EXISTS( std::filesystem::path::preferred_separator filesystem cxx17fs )
if( cxx17fs )
	add_compile_definitions( CXX17FS )
endif()

include_directories( . ../include/drea/core ../include/thirdparty ${YAML_CPP_INCLUDE_DIR} )

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Config
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY ../include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ${PROJECT_NAME}Config DESTINATION share/${PROJECT_NAME}/cmake)

export(TARGETS ${PROJECT_NAME} FILE ${PROJECT_NAME}Config.cmake)

if(APPLE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)
endif()
