# ベンチマークバイナリは build/benches/ 以下
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/benches)

# JSON benchmark
add_executable(bench_json
    bench_json.cpp
)
target_include_directories(bench_json PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(bench_json PRIVATE
    yyjson
    nlohmann_json::nlohmann_json
    nanobench::nanobench
    PRIVATE sanitizer_flags
)

# CSV benchmark
add_executable(bench_csv
    bench_csv.cpp
)
target_include_directories(bench_csv PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(bench_csv PRIVATE
    csv
    nanobench::nanobench
    PRIVATE sanitizer_flags
)
