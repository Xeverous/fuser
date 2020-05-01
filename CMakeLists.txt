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

target_sources(fuser INTERFACE
	include/fuser/fuser.hpp
)

target_include_directories(fuser INTERFACE
	${CMAKE_CURRENT_SOURCE_DIR}/include
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
