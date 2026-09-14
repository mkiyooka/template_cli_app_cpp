# メインアプリケーション用のサードパーティライブラリ定義
#
# 使用例:
#   add_external_package(fmt third_party/fmt-12.0.0
#       URL https://github.com/fmtlib/fmt/archive/refs/tags/12.0.0.tar.gz
#       URL_HASH SHA256=...
#   )
#   FetchContent_MakeAvailable(fmt)
#
#   FetchContent_Declare(fmt
#       URL https://github.com/fmtlib/fmt/archive/refs/tags/12.1.0.tar.gz
#       URL_HASH SHA256=ea7de4299689e12b6dddd392f9896f08fb0777ac7168897a244a6d6085043fea
#   )
#   FetchContent_MakeAvailable(fmt)
# 
#   FetchContent_Declare(fmt
#       GIT_REPOSITORY https://github.com/fmtlib/fmt.git
#       GIT_TAG 12.1.0
#   )
#   FetchContent_MakeAvailable(fmt)
#
#   FetchContent_Declare(yyjson
#       SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/fmt-12.1.0
#   )
#   FetchContent_MakeAvailable(fmt)

# CLI11 - Command line parser
add_external_package(CLI11 third_party/CLI11-2.6.2
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.6.2.tar.gz
    URL_HASH SHA256=c6ea6b2e5608b3ea8617999bd5f47420c71b2ebdb8dc4767c1034d1da5785711
)
FetchContent_MakeAvailable(CLI11)

# fmt - Formatting library
add_external_package(fmt third_party/fmt-12.2.0
    URL https://github.com/fmtlib/fmt/archive/refs/tags/12.2.0.tar.gz
    URL_HASH SHA256=8b852bb5aa6e7d8564f9e81394055395dd1d1936d38dfd3a17792a02bebd7af0
)
FetchContent_MakeAvailable(fmt)

# tomlplusplus - TOML configuration library
add_external_package(tomlplusplus third_party/tomlplusplus-3.4.0
    URL https://github.com/marzer/tomlplusplus/archive/refs/tags/v3.4.0.tar.gz
    URL_HASH SHA256=8517f65938a4faae9ccf8ebb36631a38c1cadfb5efa85d9a72e15b9e97d25155
)
FetchContent_MakeAvailable(tomlplusplus)

# nlohmann/json - JSON/JSONC parser
add_external_package(nlohmann_json third_party/nlohmann_json-3.12.0
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
)
FetchContent_MakeAvailable(nlohmann_json)

add_external_package(yyjson third_party/yyjson-0.12.0
    URL https://github.com/ibireme/yyjson/archive/refs/tags/0.12.0.tar.gz
    URL_HASH SHA256=b16246f617b2a136c78d73e5e2647c6f1de1313e46678062985bdcf1f40bb75d
)
FetchContent_MakeAvailable(yyjson)

# fkYAML - YAML parser (header-only)
add_external_package(fkYAML third_party/fkYAML-0.4.3
    URL https://github.com/fktn-k/fkYAML/releases/download/v0.4.3/fkYAML.tgz
    URL_HASH SHA256=2ef4c356fe3ef555694932eb6bf3de6b9893f14f425d743ff0e6a33d2465896d
)
FetchContent_MakeAvailable(fkYAML)

# Create interface library for fkYAML (header-only)
add_library(fkYAML_target INTERFACE)
target_include_directories(fkYAML_target INTERFACE ${fkyaml_SOURCE_DIR}/include)

# tl::expected - monadic error handling (header-only)
# C++23 以降では std::expected が利用可能。切り替えは include/template_cli_app_cpp/compat/expected.hpp で行う。
# tl_expected の CMakeLists.txt は BUILD_TESTING (親プロジェクトの include(CTest) で ON になる)
# に連動して自身のテストを ALL ターゲットに含めるため、明示的に無効化する。
set(EXPECTED_BUILD_TESTS OFF CACHE BOOL "" FORCE)
add_external_package(tl_expected third_party/expected-1.3.1
    URL https://github.com/TartanLlama/expected/archive/refs/tags/v1.3.1.tar.gz
    URL_HASH SHA256=9a04f4f472fbb5c30bf60402f1ca626c4a76987f867978d0b8a35d7ab3fb8fe7
)
FetchContent_MakeAvailable(tl_expected)

# tcb::span - std::span backport for C++17 (header-only)
# C++20 以降では std::span が利用可能。切り替えは include/template_cli_app_cpp/compat/span.hpp で行う。
# tcb_span の CMakeLists.txt は EXCLUDE_FROM_ALL がなく test を無条件ビルドするため、
# SOURCE_SUBDIR を存在しないディレクトリに向けて取得のみ行い、CMakeLists.txt は実行しない
add_external_package(tcb_span third_party/span
    GIT_REPOSITORY https://github.com/tcbrindle/span.git
    GIT_TAG 836dc6a0efd9849cb194e88e4aa2387436bb079b
    SOURCE_SUBDIR _skip_cmake_build
)
FetchContent_MakeAvailable(tcb_span)

# tcb::span のインターフェースライブラリを作成（header-only）
add_library(tcb_span_target INTERFACE)
target_include_directories(tcb_span_target INTERFACE ${tcb_span_SOURCE_DIR}/include)

# csv-parser - CSV reading library (streaming, memory-efficient for large files)
# csv-parser の CMakeLists.txt は programs/（csv_info, csv_stats）を無条件でビルドするため無効化する
set(CSV_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
add_external_package(csv_parser ext/csv-parser-5.3.0
    TARGET_CHECK csv
    URL https://github.com/vincentlaucsb/csv-parser/archive/refs/tags/5.3.0.tar.gz
    URL_HASH SHA256=66911a50cfb347c47bc109a52bb01b1bf7bb35184466b6bda8bca8770a41576c
)
if(NOT csv_parser_ALREADY_PROVIDED)
    FetchContent_MakeAvailable(csv_parser)
endif()
