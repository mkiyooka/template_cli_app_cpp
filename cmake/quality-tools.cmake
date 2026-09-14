# Quality management tools configuration
# Usage: include(cmake/quality-tools.cmake)

# Quality tools search paths
# pixi run 経由で実行する場合、pixi環境のbinがPATHに含まれているため
# 通常のfind_programでpixi環境のツールが優先的に発見される。
# ただし、find_programはキャッシュするため2回目以降の実行では最初の結果が使われる。
# キャッシュをリセットしたい場合は build ディレクトリを削除して再設定すること。
# ツール未検出の報告。
# 品質ツールは pixi の dev 環境（pixi install -e dev）でのみ導入されるため、
# default 環境で見つからないのは正常。dev 環境（PIXI_ENVIRONMENT_NAME=dev）で
# 見つからない場合のみ WARNING にし、それ以外は導入方法を添えた STATUS に留める。
function(_report_missing_quality_tool label)
    if("$ENV{PIXI_ENVIRONMENT_NAME}" STREQUAL "dev")
        message(WARNING "  ${label}: NOT FOUND (expected in the pixi dev environment)")
    else()
        message(STATUS "  ${label}: not found (install with: pixi install -e dev)")
    endif()
endfunction()

function(setup_quality_tools)
    # clang-format
    find_program(CLANG_FORMAT_EXE NAMES clang-format)

    # clang-tidy
    find_program(CLANG_TIDY_EXE NAMES clang-tidy)

    # cppcheck
    find_program(CPPCHECK_EXE NAMES cppcheck)

    # Display found tools
    message(STATUS "Quality tools:")
    if(CLANG_FORMAT_EXE)
        message(STATUS "  clang-format : ${CLANG_FORMAT_EXE}")
    else()
        _report_missing_quality_tool("clang-format")
    endif()

    if(CLANG_TIDY_EXE)
        message(STATUS "  clang-tidy   : ${CLANG_TIDY_EXE}")
    else()
        _report_missing_quality_tool("clang-tidy  ")
    endif()

    if(CPPCHECK_EXE)
        message(STATUS "  cppcheck     : ${CPPCHECK_EXE}")
    else()
        _report_missing_quality_tool("cppcheck    ")
    endif()

    # Set variables for parent scope
    set(CLANG_FORMAT_EXE "${CLANG_FORMAT_EXE}" PARENT_SCOPE)
    set(CLANG_TIDY_EXE "${CLANG_TIDY_EXE}" PARENT_SCOPE)
    set(CPPCHECK_EXE "${CPPCHECK_EXE}" PARENT_SCOPE)
endfunction()

# Setup quality tools targets
function(setup_quality_targets SOURCE_FILES COMPILABLE_SOURCE_FILES)
    if(CLANG_FORMAT_EXE)
        add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -i ${SOURCE_FILES}
            COMMENT "Formatting source code with clang-format"
        )
        add_custom_target(format-dry
            COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${SOURCE_FILES}
            COMMENT "Checking formatting (dry-run)"
        )
    else()
        add_custom_target(format
            COMMAND ${CMAKE_COMMAND} -E echo "clang-format not available"
            COMMENT "clang-format not found - skipping format"
        )
        add_custom_target(format-dry
            COMMAND ${CMAKE_COMMAND} -E echo "clang-format not available"
            COMMENT "clang-format not found - skipping format check"
        )
    endif()

    if(CLANG_TIDY_EXE)
        # CMAKE_CXX_CLANG_TIDY はセットしない。
        # セットするとビルド時に build/_deps のサードパーティコードまで
        # チェックされてしまうため、lint ターゲット経由での明示実行のみとする。

        # build/_deps に Checks:"-*" の .clang-tidy を生成してサードパーティを除外する。
        # run-clang-tidy がインクルードグラフを辿って _deps 以下のヘッダに
        # .clang-tidy を適用しようとするのを防ぐ。
        file(WRITE "${CMAKE_BINARY_DIR}/_deps/.clang-tidy" "Checks: \"-*\"\n")

        # Check for run-clang-tidy for parallel execution
        find_program(RUN_CLANG_TIDY_EXE NAMES run-clang-tidy)

        if(RUN_CLANG_TIDY_EXE)
            # Parallel execution with run-clang-tidy
            # clang-tidy は 1 プロセスあたり数百 MB 使うため、メモリの少ない環境では
            # -DLINT_JOBS=1 のように configure 時に明示指定して抑えられるようにする
            set(LINT_JOBS "" CACHE STRING "clang-tidy の並列実行数（空なら CPU 数の半分）")
            if(NOT LINT_JOBS)
                include(ProcessorCount)
                ProcessorCount(N)
                if(NOT N EQUAL 0)
                    math(EXPR LINT_JOBS "${N} / 2")  # Use half of available cores
                    if(LINT_JOBS LESS 1)
                        set(LINT_JOBS 1)
                    endif()
                else()
                    set(LINT_JOBS 4)
                endif()
            endif()

            add_custom_target(lint
                COMMAND ${RUN_CLANG_TIDY_EXE}
                    -p ${CMAKE_BINARY_DIR}
                    -quiet
                    -j ${LINT_JOBS}
                    ${COMPILABLE_SOURCE_FILES}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running clang-tidy (${LINT_JOBS} parallel jobs)"
                VERBATIM
            )
        else()
            # Sequential execution with clang-tidy
            add_custom_target(lint
                COMMAND ${CLANG_TIDY_EXE}
                    -p ${CMAKE_BINARY_DIR}
                    --quiet
                    ${COMPILABLE_SOURCE_FILES}
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMENT "Running clang-tidy"
                VERBATIM
            )
        endif()
    else()
        add_custom_target(lint
            COMMAND ${CMAKE_COMMAND} -E echo "clang-tidy not available"
            COMMENT "clang-tidy not found - skipping lint"
        )
    endif()

    # cppcheck targets
    if(CPPCHECK_EXE)
        # cppcheck arguments
        set(CPPCHECK_BASE_ARGS
            --enable=warning,style,performance,portability
            --std=c++17
            --platform=native
            -I include/
            --suppress=missingIncludeSystem
            --suppress=unusedFunction
            --suppress=unusedStructMember
            --suppress=ctuOneDefinitionRuleViolation
            --suppress=normalCheckLevelMaxBranches
            --suppress=unmatchedSuppression
            --suppress=useStlAlgorithm
            --inline-suppr
            --quiet
        )
        if(DEFINED CPPCHECK_ADDITIONAL_ARGS)
            string(REPLACE " " ";" CPPCHECK_EXTRA_ARGS "${CPPCHECK_ADDITIONAL_ARGS}")
            list(APPEND CPPCHECK_BASE_ARGS ${CPPCHECK_EXTRA_ARGS})
        endif()

        add_custom_target(run-cppcheck
            COMMAND ${CPPCHECK_EXE}
                ${CPPCHECK_BASE_ARGS}
                ${SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running cppcheck static analysis"
            VERBATIM
        )

        # cppcheck verbose ver.
        set(CPPCHECK_VERBOSE_ARGS ${CPPCHECK_BASE_ARGS})
        list(REMOVE_ITEM CPPCHECK_VERBOSE_ARGS "--quiet")
        list(APPEND CPPCHECK_VERBOSE_ARGS "--verbose")

        add_custom_target(run-cppcheck-verbose
            COMMAND ${CPPCHECK_EXE}
                ${CPPCHECK_VERBOSE_ARGS}
                ${SOURCE_FILES}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMENT "Running cppcheck static analysis (verbose output)"
            VERBATIM
        )
    else()
        add_custom_target(run-cppcheck
            COMMAND ${CMAKE_COMMAND} -E echo "cppcheck not available"
            COMMENT "cppcheck not found - skipping cppcheck analysis"
        )
        add_custom_target(run-cppcheck-verbose
            COMMAND ${CMAKE_COMMAND} -E echo "cppcheck not available"
            COMMENT "cppcheck not found - skipping verbose cppcheck analysis"
        )
    endif()
endfunction()
