# FetchContent で取得したライブラリのライセンスファイルを収集するスクリプト
#
# cmake -P で実行する。以下を -D で渡すこと。
#   DEPS_DIR   : FetchContent のソース展開先（FETCHCONTENT_BASE_DIR）
#   OUTPUT_DIR : ライセンスファイルのコピー先
#
# 取得済みパッケージは <DEPS_DIR>/<name>-src を走査して自動判別する。
# パッケージ名をハードコードしないため、依存ライブラリの増減に追従する。

if(NOT DEFINED DEPS_DIR OR NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "DEPS_DIR と OUTPUT_DIR を -D で指定すること")
endif()

if(NOT EXISTS "${DEPS_DIR}")
    message(STATUS "FetchContent ディレクトリが存在しない（取得済み依存なし）: ${DEPS_DIR}")
    return()
endif()

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

file(GLOB _source_dirs LIST_DIRECTORIES true "${DEPS_DIR}/*-src")

set(_collected 0)
foreach(_source_dir IN LISTS _source_dirs)
    if(NOT IS_DIRECTORY "${_source_dir}")
        continue()
    endif()

    get_filename_component(_dir_name "${_source_dir}" NAME)
    string(REGEX REPLACE "-src$" "" _package "${_dir_name}")

    file(GLOB _license_files
        "${_source_dir}/LICENSE*"
        "${_source_dir}/LICENCE*"
        "${_source_dir}/COPYING*"
    )

    foreach(_license_file IN LISTS _license_files)
        if(IS_DIRECTORY "${_license_file}")
            continue()
        endif()
        get_filename_component(_license_name "${_license_file}" NAME)
        configure_file("${_license_file}"
            "${OUTPUT_DIR}/fetchcontent_${_package}_${_license_name}" COPYONLY)
        message(STATUS "Copied license: fetchcontent_${_package}_${_license_name}")
        math(EXPR _collected "${_collected} + 1")
    endforeach()
endforeach()

message(STATUS "Collected ${_collected} license file(s) into ${OUTPUT_DIR}")
