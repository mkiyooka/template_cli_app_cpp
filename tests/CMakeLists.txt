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
target_link_libraries(test_config_manager PRIVATE config_lib command_lib CLI11::CLI11 doctest::doctest)
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
target_link_libraries(test_doctest_usage PRIVATE doctest::doctest)
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
)
add_test(
    NAME test_yyjson_wrapper
    COMMAND $<TARGET_FILE:test_yyjson_wrapper>
)
