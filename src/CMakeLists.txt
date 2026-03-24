# Add subdirectories
add_subdirectory(command)
add_subdirectory(config)

# Main executable
add_executable(cmd main.cpp)
set_target_properties(cmd PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_link_libraries(cmd PRIVATE command_lib config_lib sanitizer_flags)

# プリコンパイル済みヘッダーの設定
if(USE_PCH)
    target_precompile_headers(cmd PRIVATE
        <memory>
        <string>
        <CLI/CLI.hpp>
        <fmt/base.h>
    )
    message(STATUS "Precompiled headers enabled for executables")
endif()
