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
	LogHelpersTest.cpp
	ValidationTest.cpp
	ConfigFileTest.cpp
)

add_executable( ${PROJECT_NAME} ${SOURCES} )

set_target_properties( ${PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
drea_set_rpath( ${PROJECT_NAME} )

# core/ so tests can exercise internal pieces (json_lines_formatter,
# integrations/help/validate.h); include/drea/core so those internal headers
# resolve their flat "App.h"-style includes like they do inside core/
target_include_directories( ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../core ${CMAKE_CURRENT_SOURCE_DIR}/../include/drea/core )
target_link_libraries( ${PROJECT_NAME}
	PRIVATE
		Catch2::Catch2WithMain
		drea
)

# same format gates as the library, so the config-file cases only run for the
# formats this build actually reads (see core/CMakeLists.txt)
if( DREA_JSON_ENABLED )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE ENABLE_JSON )
endif()
if( DREA_TOML_ENABLED )
	target_compile_definitions( ${PROJECT_NAME} PRIVATE ENABLE_TOML )
endif()

clang_format_setup( ${SOURCES} )

include( GNUInstallDirs )
install( TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

add_test(
	NAME ${PROJECT_NAME}
	COMMAND ${PROJECT_NAME}
)

# End-to-end checks on the example binaries: the exit code of a misused command
# line, and the machine-readable output against the published JSON Schemas. Only
# with the examples built, since it runs them; skipped when python3 is missing.
if( BUILD_EXAMPLES AND NOT WIN32 )
	add_test(
		NAME drea-cli-checks
		COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/cli-checks.sh $<TARGET_FILE_DIR:hello> ${CMAKE_CURRENT_SOURCE_DIR}/..
	)
	set_tests_properties( drea-cli-checks PROPERTIES SKIP_RETURN_CODE 77 )
endif()
