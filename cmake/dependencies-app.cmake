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
add_external_package(CLI11 third_party/CLI11-2.5.0
    URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.5.0.tar.gz
    URL_HASH SHA256=17e02b4cddc2fa348e5dbdbb582c59a3486fa2b2433e70a0c3bacb871334fd55
)
FetchContent_MakeAvailable(CLI11)

# fmt - Formatting library
add_external_package(fmt third_party/fmt-12.0.0
    URL https://github.com/fmtlib/fmt/archive/refs/tags/12.0.0.tar.gz
    URL_HASH SHA256=aa3e8fbb6a0066c03454434add1f1fc23299e85758ceec0d7d2d974431481e40
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
add_external_package(fkYAML third_party/fkYAML-0.4.2
    URL https://github.com/fktn-k/fkYAML/releases/download/v0.4.2/fkYAML.tgz
    URL_HASH SHA256=6fbdbd94094b467ea37a64ccfe042588353feda097b1a5348f8cd12b914ad2cd
)
FetchContent_MakeAvailable(fkYAML)

# Create interface library for fkYAML (header-only)
add_library(fkYAML_target INTERFACE)
target_include_directories(fkYAML_target INTERFACE ${fkyaml_SOURCE_DIR}/include)

# spdlog - Fast C++ logging library
# SPDLOG_FMT_EXTERNAL=ON: spdlog のバンドル fmt を使わず、
# 上記で導入済みの fmt::fmt を共有する（ODR違反・二重定義を防ぐ）
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
add_external_package(spdlog third_party/spdlog-1.17.0
    URL https://github.com/gabime/spdlog/archive/refs/tags/v1.17.0.tar.gz
    URL_HASH SHA256=d8862955c6d74e5846b3f580b1605d2428b11d97a410d86e2fb13e857cd3a744
)
FetchContent_MakeAvailable(spdlog)

# tl::expected - monadic error handling (header-only)
add_external_package(tl_expected third_party/expected-1.1.0
    URL https://github.com/TartanLlama/expected/archive/refs/tags/v1.1.0.tar.gz
    URL_HASH SHA256=1db357f46dd2b24447156aaf970c4c40a793ef12a8a9c2ad9e096d9801368df6
)
FetchContent_MakeAvailable(tl_expected)
