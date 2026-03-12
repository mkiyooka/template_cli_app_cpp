# Source-based code coverage targets (clang / llvm-cov)
# Usage: cmake --preset=coverage && cmake --build build-coverage && cmake --build build-coverage --target coverage
#
# 生成されるターゲット:
#   coverage        - テスト実行 → プロファイルデータ統合 → テキストレポート + HTML レポート生成
#   coverage-report - HTML レポートのパスを表示する

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

# カバレッジ出力ディレクトリ・ファイル
set(COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage-html")
set(COVERAGE_PROFDATA   "${CMAKE_BINARY_DIR}/coverage.profdata")

# llvm-cov に渡すオブジェクトフラグ（1つ目がメイン、2つ目以降は --object=）
set(_FIRST_BIN $<TARGET_FILE:test_config_manager>)
set(_LLVM_COV_OBJECT_FLAGS
    "--object=$<TARGET_FILE:test_doctest_usage>"
    "--object=$<TARGET_FILE:test_yyjson_wrapper>"
)

add_custom_target(coverage
    # 1. 古い profraw を削除
    COMMAND ${CMAKE_COMMAND} -E echo "=== Cleaning old profile data ==="
    COMMAND sh -c "rm -f '${CMAKE_BINARY_DIR}/coverage-'*.profraw 2>/dev/null; true"
    # 2. テストバイナリを直接実行（LLVM_PROFILE_FILE で出力先を指定）
    COMMAND ${CMAKE_COMMAND} -E echo "=== Running tests with coverage instrumentation ==="
    COMMAND ${CMAKE_COMMAND} -E env
        "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage-%p.profraw"
        $<TARGET_FILE:test_config_manager>
    COMMAND ${CMAKE_COMMAND} -E env
        "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage-%p.profraw"
        $<TARGET_FILE:test_doctest_usage>
    COMMAND ${CMAKE_COMMAND} -E env
        "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage-%p.profraw"
        $<TARGET_FILE:test_yyjson_wrapper>
    # 3. プロファイルデータを統合
    COMMAND ${CMAKE_COMMAND} -E echo "=== Merging profile data ==="
    COMMAND sh -c "${LLVM_PROFDATA_EXE} merge --sparse '${CMAKE_BINARY_DIR}/coverage-'*.profraw -o '${COVERAGE_PROFDATA}'"
    # 4. テキストレポート（ターミナル）
    COMMAND ${CMAKE_COMMAND} -E echo "=== Coverage report ==="
    COMMAND sh -c "${LLVM_COV_EXE} report \
        $<TARGET_FILE:test_config_manager> \
        --object=$<TARGET_FILE:test_doctest_usage> \
        --object=$<TARGET_FILE:test_yyjson_wrapper> \
        --instr-profile=${COVERAGE_PROFDATA} \
        --no-warn \
        \"--ignore-filename-regex=.*/build-coverage/.*|.*/third_party/.*|.*/.pixi/.*|.*/tests/.*\" \
        2>/dev/null"
    # 5. HTML レポート生成
    COMMAND ${CMAKE_COMMAND} -E echo "=== Generating HTML report: ${COVERAGE_OUTPUT_DIR} ==="
    COMMAND ${CMAKE_COMMAND} -E make_directory ${COVERAGE_OUTPUT_DIR}
    COMMAND sh -c "${LLVM_COV_EXE} show \
        $<TARGET_FILE:test_config_manager> \
        --object=$<TARGET_FILE:test_doctest_usage> \
        --object=$<TARGET_FILE:test_yyjson_wrapper> \
        --instr-profile=${COVERAGE_PROFDATA} \
        --format=html \
        --output-dir=${COVERAGE_OUTPUT_DIR} \
        --no-warn \
        \"--ignore-filename-regex=.*/build-coverage/.*|.*/third_party/.*|.*/.pixi/.*|.*/tests/.*\" \
        2>/dev/null"
    COMMAND ${CMAKE_COMMAND} -E echo "=== HTML report: ${COVERAGE_OUTPUT_DIR}/index.html ==="
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running coverage analysis"
    VERBATIM
    DEPENDS test_config_manager test_doctest_usage test_yyjson_wrapper
)

# HTML レポートのパスを表示するターゲット
add_custom_target(coverage-report
    COMMAND ${CMAKE_COMMAND} -E echo "Coverage HTML report: ${COVERAGE_OUTPUT_DIR}/index.html"
    COMMENT "Coverage HTML report path"
)
