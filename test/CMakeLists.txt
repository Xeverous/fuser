add_executable(fuser_test)

target_sources(fuser_test PRIVATE
	main.cpp
	tests.cpp
)

target_include_directories(fuser_test PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(fuser_test PRIVATE cxx_std_11)

target_compile_options(fuser_test PRIVATE
	$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wpedantic>
	$<$<CXX_COMPILER_ID:Clang>:-Wall -Wpedantic>
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
)

target_link_libraries(fuser_test PRIVATE fuser)
