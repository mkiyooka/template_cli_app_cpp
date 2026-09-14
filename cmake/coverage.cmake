# Source-based code coverage targets (clang / llvm-cov)
# Usage: cmake --preset=coverage && cmake --build build-coverage && cmake --build build-coverage --target coverage
#
# 生成されるターゲット:
#   coverage        - テスト実行 → プロファイルデータ統合 → テキストレポート + HTML レポート生成
#   coverage-report - HTML レポートのパスを表示する
#
# 呼び出し側の責務:
#   include() する前に COVERAGE_TEST_TARGETS に実行するテストターゲット名を設定すること。
#   （テストターゲット定義後、つまり add_subdirectory(tests) より後に include すること）
#
#   set(COVERAGE_TEST_TARGETS test_foo test_bar)
#   include(cmake/coverage.cmake)
#
#   除外パターンを足したい場合は COVERAGE_EXTRA_IGNORE_REGEX に
#   正規表現を追加する（"|" 区切りで連結される）。
#
#   set(COVERAGE_EXTRA_IGNORE_REGEX ".*/benches/.*")

if(NOT ENABLE_COVERAGE)
    return()
endif()

if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
    return()
endif()

# ツール検索
find_program(LLVM_PROFDATA_EXE NAMES llvm-profdata)
find_program(LLVM_COV_EXE      NAMES llvm-cov)

if(LLVM_PROFDATA_EXE)
    message(STATUS "llvm-profdata: ${LLVM_PROFDATA_EXE}")
else()
    message(WARNING "llvm-profdata: NOT FOUND - coverage target will not work")
endif()

if(LLVM_COV_EXE)
    message(STATUS "llvm-cov     : ${LLVM_COV_EXE}")
else()
    message(WARNING "llvm-cov     : NOT FOUND - coverage target will not work")
endif()

if(NOT DEFINED COVERAGE_TEST_TARGETS)
    message(FATAL_ERROR
        "COVERAGE_TEST_TARGETS が未設定。cmake/coverage.cmake を include する前に "
        "実行するテストターゲット名を設定すること。")
endif()

# カバレッジ出力ディレクトリ・ファイル
set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage-html")
set(COVERAGE_PROFDATA   "${CMAKE_BINARY_DIR}/coverage.profdata")

# レポート対象外にするパス
set(_ignore_regex ".*/build-coverage/.*|.*/third_party/.*|.*/_deps/.*|.*/.pixi/.*|.*/tests/.*|.*/benches/.*|.*/examples/.*")
if(DEFINED COVERAGE_EXTRA_IGNORE_REGEX)
    string(JOIN "|" _ignore_regex ${_ignore_regex} ${COVERAGE_EXTRA_IGNORE_REGEX})
endif()

# COVERAGE_TEST_TARGETS から
#   - 各テストバイナリの実行コマンド
#   - llvm-cov に渡すオブジェクト指定（1つ目がメイン、2つ目以降は --object=）
# を組み立てる
set(_coverage_run_commands "")
set(_coverage_object_flags "")
set(_coverage_first_bin "")
foreach(_target IN LISTS COVERAGE_TEST_TARGETS)
    if(NOT TARGET ${_target})
        message(FATAL_ERROR "COVERAGE_TEST_TARGETS: ターゲット '${_target}' が存在しない")
    endif()
    list(APPEND _coverage_run_commands
        COMMAND ${CMAKE_COMMAND} -E env
            "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage-%p.profraw"
            $<TARGET_FILE:${_target}>
    )
    if(_coverage_first_bin STREQUAL "")
        set(_coverage_first_bin $<TARGET_FILE:${_target}>)
    else()
        list(APPEND _coverage_object_flags "--object=$<TARGET_FILE:${_target}>")
    endif()
endforeach()

add_custom_target(coverage
    # 1. 古い profraw を削除
    COMMAND ${CMAKE_COMMAND} -E echo "=== Cleaning old profile data ==="
    COMMAND sh -c "rm -f '${CMAKE_BINARY_DIR}/coverage-'*.profraw 2>/dev/null; true"
    # 2. テストバイナリを直接実行（LLVM_PROFILE_FILE で出力先を指定）
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running tests with coverage instrumentation ==="
    ${_coverage_run_commands}
    # 3. プロファイルデータを統合
    COMMAND ${CMAKE_COMMAND} -E echo "=== Merging profile data ==="
    COMMAND sh -c "${LLVM_PROFDATA_EXE} merge --sparse '${CMAKE_BINARY_DIR}/coverage-'*.profraw -o '${COVERAGE_PROFDATA}'"
    # 4. テキストレポート（ターミナル）
    COMMAND ${CMAKE_COMMAND} -E echo "=== Coverage report ==="
    COMMAND ${LLVM_COV_EXE} report
        ${_coverage_first_bin}
        ${_coverage_object_flags}
        --instr-profile=${COVERAGE_PROFDATA}
        --no-warn
        "--ignore-filename-regex=${_ignore_regex}"
    # 5. HTML レポート生成
    COMMAND ${CMAKE_COMMAND} -E echo "=== Generating HTML report: ${COVERAGE_OUTPUT_DIR} ==="
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
    COMMAND ${LLVM_COV_EXE} show
        ${_coverage_first_bin}
        ${_coverage_object_flags}
        --instr-profile=${COVERAGE_PROFDATA}
        --format=html
        --output-dir=${COVERAGE_OUTPUT_DIR}
        --no-warn
        "--ignore-filename-regex=${_ignore_regex}"
    COMMAND ${CMAKE_COMMAND} -E echo "=== HTML report: ${COVERAGE_OUTPUT_DIR}/index.html ==="
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Running coverage analysis"
    VERBATIM
    DEPENDS ${COVERAGE_TEST_TARGETS}
)

# HTML レポートのパスを表示するターゲット
add_custom_target(coverage-report
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage HTML report: ${COVERAGE_OUTPUT_DIR}/index.html"
    COMMENT "Coverage HTML report path"
)
