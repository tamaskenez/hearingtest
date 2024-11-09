file(GLOB_RECURSE sources CONFIGURE_DEPENDS *.h *.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_library(lib STATIC ${sources})
target_link_libraries(lib PUBLIC
    absl::check
    absl::log
    absl::log_initialize
    absl::cleanup
    absl::strings
    fmt::fmt
)

target_precompile_headers(lib PRIVATE pch.h)
target_include_directories(lib PUBLIC ..)

