# テストバイナリは build/tests/ 以下
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

# Config manager test
add_executable(test_config_manager
    test_config_manager.cpp
)
target_include_directories(test_config_manager PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src/config
    ${PROJECT_SOURCE_DIR}/tests
)
target_link_libraries(test_config_manager PRIVATE config_lib command_lib CLI11::CLI11 doctest::doctest PRIVATE sanitizer_flags)
add_test(
    NAME test_config_manager
    COMMAND $<TARGET_FILE:test_config_manager>
)

# doctest 記述パターンのサンプルテスト
add_executable(test_doctest_usage
    test_doctest_usage.cpp
)
target_include_directories(test_doctest_usage PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)
target_link_libraries(test_doctest_usage PRIVATE doctest::doctest PRIVATE sanitizer_flags)
add_test(
    NAME test_doctest_usage
    COMMAND $<TARGET_FILE:test_doctest_usage>
)

# csv_wrapper test
add_executable(test_csv_wrapper
    test_csv_wrapper.cpp
)
target_include_directories(test_csv_wrapper PRIVATE
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
)
target_link_libraries(test_csv_wrapper PRIVATE
    csv
    tl::expected
    doctest::doctest
    PRIVATE sanitizer_flags
)
add_test(
    NAME test_csv_wrapper
    COMMAND $<TARGET_FILE:test_csv_wrapper>
)

# yyjson_wrapper test
add_executable(test_yyjson_wrapper
    test_yyjson_wrapper.cpp
)
target_include_directories(test_yyjson_wrapper PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(test_yyjson_wrapper PRIVATE
    yyjson
    nlohmann_json::nlohmann_json
    doctest::doctest
    PRIVATE sanitizer_flags
)
add_test(
    NAME test_yyjson_wrapper
    COMMAND $<TARGET_FILE:test_yyjson_wrapper>
)

# ==============================================================================
# System Tests (CLI バイナリの end-to-end テスト)
# ==============================================================================

set(SYSTEM_TESTS "")

# add サブコマンド（引数のみ）
add_test(
    NAME system_add_args
    COMMAND $<TARGET_FILE:cmd> add 10 20
)
set_tests_properties(system_add_args PROPERTIES
    PASS_REGULAR_EXPRESSION "10 \\+ 20 = 30"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_add_args)

# subtract サブコマンド（引数のみ）
add_test(
    NAME system_subtract_args
    COMMAND $<TARGET_FILE:cmd> subtract 15 5
)
set_tests_properties(system_subtract_args PROPERTIES
    PASS_REGULAR_EXPRESSION "15 - 5 = 10"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_subtract_args)

# multiply サブコマンド（引数のみ）
add_test(
    NAME system_multiply_args
    COMMAND $<TARGET_FILE:cmd> multiply 3 7
)
set_tests_properties(system_multiply_args PROPERTIES
    PASS_REGULAR_EXPRESSION "3 \\* 7 = 21"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_multiply_args)

# divide サブコマンド（正常系）
add_test(
    NAME system_divide_args
    COMMAND $<TARGET_FILE:cmd> divide 100 4
)
set_tests_properties(system_divide_args PROPERTIES
    PASS_REGULAR_EXPRESSION "100 / 4 = 25"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_divide_args)

# divide ゼロ除算（バリデーションエラー）
add_test(
    NAME system_divide_by_zero
    COMMAND $<TARGET_FILE:cmd> divide 10 0
)
set_tests_properties(system_divide_by_zero PROPERTIES
    WILL_FAIL TRUE
    FAIL_REGULAR_EXPRESSION "Segmentation"
)
list(APPEND SYSTEM_TESTS system_divide_by_zero)

# TOML 設定ファイル指定
add_test(
    NAME system_config_toml
    COMMAND $<TARGET_FILE:cmd> --config ${CMAKE_SOURCE_DIR}/config/example.toml add 10 20
)
set_tests_properties(system_config_toml PROPERTIES
    PASS_REGULAR_EXPRESSION "title: Example TOML Configuration"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_config_toml)

# JSON 設定ファイル指定
add_test(
    NAME system_config_json
    COMMAND $<TARGET_FILE:cmd> --config ${CMAKE_SOURCE_DIR}/config/example.json add 10 20
)
set_tests_properties(system_config_json PROPERTIES
    PASS_REGULAR_EXPRESSION "title: Example JSONC Configuration"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_config_json)

# YAML 設定ファイル指定
add_test(
    NAME system_config_yaml
    COMMAND $<TARGET_FILE:cmd> --config ${CMAKE_SOURCE_DIR}/config/example.yaml add 10 20
)
set_tests_properties(system_config_yaml PROPERTIES
    PASS_REGULAR_EXPRESSION "title: Example YAML Configuration"
    FAIL_REGULAR_EXPRESSION "Error|Segmentation"
)
list(APPEND SYSTEM_TESTS system_config_yaml)

# ヘルプ表示
add_test(
    NAME system_help
    COMMAND $<TARGET_FILE:cmd> --help
)
set_tests_properties(system_help PROPERTIES
    PASS_REGULAR_EXPRESSION "subcommand"
)
list(APPEND SYSTEM_TESTS system_help)

# 複数 --config 指定（後勝ちマージ）
add_test(
    NAME system_multi_config
    COMMAND $<TARGET_FILE:cmd>
        --config ${CMAKE_SOURCE_DIR}/tests/fixtures/base.toml
        --config ${CMAKE_SOURCE_DIR}/tests/fixtures/override.toml
        add 1 2
)
set_tests_properties(system_multi_config PROPERTIES
    PASS_REGULAR_EXPRESSION "title: Override"
    FAIL_REGULAR_EXPRESSION "Segmentation"
)
list(APPEND SYSTEM_TESTS system_multi_config)

# マニフェストファイル (.conf)
add_test(
    NAME system_manifest
    COMMAND $<TARGET_FILE:cmd>
        --config ${CMAKE_SOURCE_DIR}/tests/fixtures/multi.conf
        add 1 2
)
set_tests_properties(system_manifest PROPERTIES
    PASS_REGULAR_EXPRESSION "title: Override"
    FAIL_REGULAR_EXPRESSION "Segmentation"
)
list(APPEND SYSTEM_TESTS system_manifest)

# システムテストにラベルを付与（memcheck 除外用）
set_tests_properties(${SYSTEM_TESTS} PROPERTIES LABELS "system")
