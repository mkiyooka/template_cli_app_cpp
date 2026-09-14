# カスタムターゲット定義
#
# 呼び出し側の責務:
#   show-help の "Main applications" セクションに出す行を SHOW_HELP_MAIN_TARGETS に設定する。
#   未設定の場合、そのセクションは出力されない（ライブラリのみのプロジェクト等）。
#
#   set(SHOW_HELP_MAIN_TARGETS
#       "cmd                   - Main CLI application"
#   )
#   include(cmake/custom-targets.cmake)

# cmake --build build --target test テスト実行
# cmake --build build --target run-tests テスト実行 + 失敗時に詳細出力
add_custom_target(run-tests
    COMMAND ctest --output-on-failure
    COMMENT "Running all tests"
)

# show-help の "Main applications" セクションを組み立てる
set(_show_help_main_section "")
if(SHOW_HELP_MAIN_TARGETS)
    list(APPEND _show_help_main_section
        COMMAND ${CMAKE_COMMAND} -E echo "Main applications:"
    )
    foreach(_line IN LISTS SHOW_HELP_MAIN_TARGETS)
        list(APPEND _show_help_main_section
            COMMAND ${CMAKE_COMMAND} -E echo "  ${_line}"
        )
    endforeach()
    list(APPEND _show_help_main_section
        COMMAND ${CMAKE_COMMAND} -E echo ""
    )
endif()

# ヘルプターゲット
add_custom_target(show-help
    COMMAND ${CMAKE_COMMAND} -E echo "=== Available Build Targets ==="
    COMMAND ${CMAKE_COMMAND} -E echo ""
    ${_show_help_main_section}
    COMMAND ${CMAKE_COMMAND} -E echo "Development targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  run-tests             - Run all tests"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Code quality targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  format                - Format code with clang-format"
    COMMAND ${CMAKE_COMMAND} -E echo "  format-dry            - Check formatting without changes"
    COMMAND ${CMAKE_COMMAND} -E echo "  lint                  - Lint code with clang-tidy"
    COMMAND ${CMAKE_COMMAND} -E echo "  run-cppcheck          - Run cppcheck static analysis"
    COMMAND ${CMAKE_COMMAND} -E echo "  run-cppcheck-verbose  - Run cppcheck with verbose output"
    COMMENT "Showing available build targets"
)
