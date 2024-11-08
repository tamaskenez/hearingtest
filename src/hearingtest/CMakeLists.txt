file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(hearingtest ${sources})
target_link_libraries(hearingtest PRIVATE
	jucemodules
	absl::check
	absl::log
	absl::log_initialize
	absl::cleanup
	absl::strings
	fmt::fmt
)
target_precompile_headers(hearingtest PRIVATE pch.h)
