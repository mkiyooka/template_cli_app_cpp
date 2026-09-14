# テストフレームワーク用のサードパーティライブラリ定義

# doctest - Testing framework
set(CMAKE_POLICY_DEFAULT_CMP0091 NEW)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
add_external_package(doctest ext/doctest-2.5.3
    TARGET_CHECK doctest::doctest
    URL https://github.com/doctest/doctest/archive/refs/tags/v2.5.3.tar.gz
    URL_HASH SHA256=174ebc4e769928959614789c5b4e9c3d0a0f81a62bb608756b127bfebfb21331
)
if(NOT doctest_ALREADY_PROVIDED)
    FetchContent_MakeAvailable(doctest)
endif()

# nanobench - Benchmarking library (header-only)
# 利用側（benches/*.cpp）が ANKERL_NANOBENCH_IMPLEMENT を定義して実装を取り込むため、
# nanobench 側の CMakeLists.txt（static lib のビルド。古い cmake_minimum_required で
# 非推奨警告を出す）は実行せず、SOURCE_SUBDIR で取得のみ行ってヘッダを INTERFACE で公開する
add_external_package(nanobench ext/nanobench-4.3.11
    TARGET_CHECK nanobench::nanobench
    URL https://github.com/martinus/nanobench/archive/refs/tags/v4.3.11.tar.gz
    URL_HASH SHA256=53a5a913fa695c23546661bf2cd22b299e10a3e994d9ed97daf89b5cada0da70
    SOURCE_SUBDIR _skip_cmake_build
)
if(NOT nanobench_ALREADY_PROVIDED)
    FetchContent_MakeAvailable(nanobench)
    add_library(nanobench_header_only INTERFACE)
    add_library(nanobench::nanobench ALIAS nanobench_header_only)
    target_include_directories(nanobench_header_only SYSTEM INTERFACE ${nanobench_SOURCE_DIR}/src/include)
endif()

# Mark doctest as system library to exclude it from clang-tidy checks
if(TARGET doctest)
    get_target_property(doctest_include_dirs doctest INTERFACE_INCLUDE_DIRECTORIES)
    if(doctest_include_dirs)
        set_target_properties(doctest PROPERTIES
            INTERFACE_SYSTEM_INCLUDE_DIRECTORIES "${doctest_include_dirs}"
        )
    endif()
endif()
