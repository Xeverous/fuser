# https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
# https://cliutils.gitlab.io/modern-cmake/

# 3.13 is required for relative source file paths
cmake_minimum_required(VERSION 3.13...3.16)

project(fuser
	DESCRIPTION "Header-only library for automatic (de)serialization of C++ types to/from JSON."
	LANGUAGES CXX)

##############################################################################
# options for customizing the build

option(FUSER_ONLY_CXX_11 "build fuser library with C++11 only" OFF)
option(FUSER_BUILD_TESTS "build fuser library tests" OFF)
option(FUSER_USE_EXTERNAL_JSON
"ON: use find_package() to locate nlohmann/json; \
OFF: use add_subdirectory() with FUSER_NLOHMANN_JSON_ROOT" ON)

##############################################################################
# dependencies and target definitions

if(FUSER_USE_EXTERNAL_JSON)
	find_package(nlohmann_json 3.0.0 REQUIRED)
else()
	if(NOT ${FUSER_NLOHMANN_JSON_ROOT})
		message(FATAL_ERROR "non-external JSON enabled but FUSER_NLOHMANN_JSON_ROOT was not set")
	endif()

	set(JSON_BuildTests OFF CACHE INTERNAL "")
	add_subdirectory(${FUSER_NLOHMANN_JSON_ROOT} ${CMAKE_BINARY_DIR}/json)
endif()

# 1.56 introduced typeindex library, used in tests
find_package(Boost 1.56 REQUIRED)

add_library(fuser INTERFACE)
add_library(fuser::fuser ALIAS fuser)

target_sources(fuser INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/fuser/fuser.hpp>
)

target_include_directories(fuser INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(FUSER_ONLY_CXX_11)
	message(STATUS "fuser standard: C++11")
	target_compile_features(fuser INTERFACE cxx_std_11)
else()
	message(STATUS "fuser standard: C++17")
	target_compile_features(fuser INTERFACE cxx_std_17)
endif()

target_link_libraries(fuser INTERFACE
	nlohmann_json::nlohmann_json
	Boost::boost
)

if(FUSER_BUILD_TESTS)
	add_subdirectory(test)
endif()

include(GNUInstallDirs)
set(config_install_dir "${CMAKE_INSTALL_DATADIR}/cmake/fuser")
set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(version_config "${generated_dir}/fuserConfigVersion.cmake")
set(project_config "${generated_dir}/fuserConfig.cmake")
set(TARGETS_EXPORT_NAME "fuserTargets")
set(namespace "fuser::")

include(CMakePackageConfigHelpers)
#write_basic_package_version_file(
#	"${version_config}"
#	VERSION ${PROJECT_VERSION}
#	COMPATIBILITY AnyNewerVersion
#)
configure_package_config_file(
	"cmake/Config.cmake.in"
	"${project_config}"
	INSTALL_DESTINATION "${config_install_dir}"
)
install(
	TARGETS fuser
	EXPORT "${TARGETS_EXPORT_NAME}"
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/fuser"
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(FILES "${project_config}" #"${version_config}"
	DESTINATION "${config_install_dir}"
)
install(
	EXPORT "${TARGETS_EXPORT_NAME}"
	NAMESPACE "${namespace}"
	DESTINATION "${config_install_dir}")

# uninstall target
if(NOT TARGET uninstall)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE
		@ONLY)
	add_custom_target(uninstall COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()