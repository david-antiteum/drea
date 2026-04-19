cmake_minimum_required( VERSION 3.21 )

project( drea VERSION 0.30.0 DESCRIPTION "Drea Framework" LANGUAGES CXX )

# Options are declared in the top-level CMakeLists when drea is the root
# project. When consumed via add_subdirectory they may already be set by the
# parent; declare here as a fallback so this subdir is usable standalone.
if( NOT DEFINED BUILD_REST_USE )
	option( BUILD_REST_USE "Build support to access REST services" ON )
endif()
if( NOT DEFINED ENABLE_AWS )
	option( ENABLE_AWS "Build support for AWS Secrets Manager as a config source" OFF )
endif()

#
# Dependencies
#

find_package( fmt          CONFIG REQUIRED )
find_package( spdlog       CONFIG REQUIRED )
find_package( yaml-cpp     CONFIG REQUIRED )
find_package( tl-expected  CONFIG REQUIRED )
find_package( Boost        REQUIRED )

if( TARGET yaml-cpp::yaml-cpp )
	set( YAML_TARGET yaml-cpp::yaml-cpp )
else()
	set( YAML_TARGET yaml-cpp )
endif()

if( BUILD_REST_USE )
	if( USE_SYSTEM_LIBS )
		find_package( PkgConfig REQUIRED )
		pkg_search_module( liburiparser REQUIRED IMPORTED_TARGET GLOBAL liburiparser )
		add_library( uriparser::uriparser ALIAS PkgConfig::liburiparser )
	else()
		find_package( uriparser CONFIG REQUIRED )
	endif()
	find_package( nlohmann_json CONFIG REQUIRED )
	set( DREA_JSON_ENABLED ON )
else()
	find_package( nlohmann_json CONFIG )
	if( nlohmann_json_FOUND )
		set( DREA_JSON_ENABLED ON )
	else()
		set( DREA_JSON_ENABLED OFF )
		message( STATUS "nlohmann_json not found. JSON config-file support disabled." )
	endif()
endif()

if( ENABLE_AWS )
	find_package( AWSSDK CONFIG REQUIRED COMPONENTS secretsmanager )
endif()

find_path( CPPTOML_INCLUDE_DIR cpptoml.h )
if( CPPTOML_INCLUDE_DIR STREQUAL "CPPTOML_INCLUDE_DIR-NOTFOUND" )
	set( DREA_TOML_ENABLED OFF )
	message( STATUS "cpptoml not found. TOML config-file support disabled." )
else()
	set( DREA_TOML_ENABLED ON )
endif()

#
# Library target
#

add_library( ${PROJECT_NAME}
	App.cpp
	Config.cpp
	Commander.cpp
	Option.cpp
	Command.cpp
)

set_target_properties( ${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	VERSION   ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

drea_set_rpath( ${PROJECT_NAME} )

target_include_directories( ${PROJECT_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/thirdparty>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/thirdparty>
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_CURRENT_SOURCE_DIR}/../include/drea/core
)

# spdlog appears in public headers (Config::setupLogger returns
# std::shared_ptr<spdlog::logger>), so it must be PUBLIC. fmt is a header-only
# transitive via spdlog but is also used directly in examples -> PUBLIC.
target_link_libraries( ${PROJECT_NAME}
	PUBLIC
		fmt::fmt
		spdlog::spdlog
		${YAML_TARGET}
		tl::expected
	PRIVATE
		Boost::headers
)

if( BUILD_REST_USE )
	target_link_libraries( ${PROJECT_NAME} PRIVATE uriparser::uriparser )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE ENABLE_REST_USE )
endif()
if( DREA_JSON_ENABLED )
	target_link_libraries( ${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE ENABLE_JSON )
endif()
if( DREA_TOML_ENABLED )
	target_include_directories( ${PROJECT_NAME} PRIVATE ${CPPTOML_INCLUDE_DIR} )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE ENABLE_TOML )
endif()
if( ENABLE_AWS )
	target_link_libraries( ${PROJECT_NAME} PRIVATE ${AWSSDK_LINK_LIBRARIES} )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE ENABLE_AWS )
endif()

if( WIN32 )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE YAML_CPP_DLL )
endif()

if( MSVC )
	set_source_files_properties( Config.cpp PROPERTIES COMPILE_FLAGS /bigobj )
endif()

if( BUILD_SHARED_LIBS )
	target_compile_definitions( ${PROJECT_NAME}
		PUBLIC DREA_CORE_DLL
		PRIVATE DREA_CORE_EXPORTS
	)
	message( STATUS "Drea dynamic build" )
else()
	message( STATUS "Drea static build" )
endif()

include( CheckCXXSymbolExists )
CHECK_CXX_SYMBOL_EXISTS( std::filesystem::path::preferred_separator filesystem cxx17fs )
if( cxx17fs )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE CXX17FS )
endif()

#
# Install + package config
#

include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

install( TARGETS ${PROJECT_NAME}
	EXPORT dreaTargets
	ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install( DIRECTORY ../include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} )

install( EXPORT dreaTargets
	FILE dreaTargets.cmake
	DESTINATION ${CMAKE_INSTALL_DATADIR}/drea/cmake
)

configure_package_config_file(
	${CMAKE_SOURCE_DIR}/cmake/dreaConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/dreaConfig.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/drea/cmake
)
write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/dreaConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)
install( FILES
	${CMAKE_CURRENT_BINARY_DIR}/dreaConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/dreaConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_DATADIR}/drea/cmake
)
