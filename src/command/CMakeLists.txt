# Command sources
set(COMMAND_SOURCES
    cli.cpp
    subcommand.cpp
)

# Create command library
add_library(command_lib STATIC ${COMMAND_SOURCES})
target_include_directories(command_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(command_lib PUBLIC CLI11::CLI11 fmt::fmt yyjson spdlog::spdlog)
target_link_libraries(command_lib PRIVATE sanitizer_flags)
