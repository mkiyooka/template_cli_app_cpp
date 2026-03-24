# Config sources
set(CONFIG_SOURCES
    config_loader.cpp
    config_file_loader.cpp
    config_manager.cpp
)

# Create config library
add_library(config_lib STATIC ${CONFIG_SOURCES})
target_include_directories(config_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(config_lib PUBLIC
    CLI11::CLI11
    tomlplusplus::tomlplusplus
    nlohmann_json::nlohmann_json
    fkYAML_target
    fmt::fmt
)
target_link_libraries(config_lib PRIVATE sanitizer_flags)
