# ローカルディレクトリが存在する場合はそれを使用し、
# 存在しない場合はHTTP/FTPから取得する関数
#
# 使用例(URL):
#   add_external_package(CLI11 ext/CLI11-2.5.0
#       TARGET_CHECK CLI11::CLI11
#       URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.5.0.tar.gz
#       URL_HASH SHA256=abc123...
#   )
#   if(NOT CLI11_ALREADY_PROVIDED)
#       FetchContent_MakeAvailable(CLI11)
#   endif()
#
# 使用例(Git):
#   add_external_package(CLI11 ext/CLI11-2.5.0
#       TARGET_CHECK CLI11::CLI11
#       GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
#       GIT_TAG v2.5.0
#   )
#   if(NOT CLI11_ALREADY_PROVIDED)
#       FetchContent_MakeAvailable(CLI11)
#   endif()
#
# 引数:
#   LIBRARY_NAME: ライブラリ名
#   LOCAL_PATH: ローカルディレクトリパス（プロジェクトルートからの相対パス）
#   TARGET_CHECK: 導入済みかどうかを判定するターゲット名（任意）
#   URL: ダウンロードURL（HTTP/FTP取得時、推奨）
#   URL_HASH: チェックサムハッシュ（URL指定時は必須、SHA256=...形式）
#   GIT_REPOSITORY: GitリポジトリURL（Git取得時、常にshallow clone）
#   GIT_TAG: Gitタグまたはブランチ名
#   SOURCE_SUBDIR: FetchContent_MakeAvailable が add_subdirectory する相対パス（任意）。
#     ヘッダのみ使いたいライブラリで相手の CMakeLists.txt を実行したくない場合は、
#     存在しないディレクトリ名（例: _skip_cmake_build）を指定する。MakeAvailable は
#     その場所に CMakeLists.txt が無ければ add_subdirectory をスキップし、取得だけを行う。
#     非推奨の FetchContent_Populate（CMP0169）を使わずに「取得のみ」を実現する定石。
#
# 排他制御（呼び出し元プロジェクトとの干渉回避）:
#   cliconf は FetchContent で他プロジェクトに取り込まれることを想定しているため、
#   cliconf が依存する fmt / nlohmann_json / yyjson 等は、取り込み側プロジェクトが
#   同じライブラリを別バージョンで FetchContent 済み、というケースが起こり得る。
#   その場合に cliconf 側が無条件で FetchContent_Declare すると、
#     - 同名ターゲットの二重定義エラー
#     - 宣言順によって意図しないバージョンが暗黙的に採用される
#   といった問題が発生する。
#
#   TARGET_CHECK にターゲット名を指定すると、そのターゲットが呼び出し時点で
#   既に存在する場合（取り込み側が add_subdirectory(cliconf) / FetchContent_MakeAvailable(cliconf)
#   より前に自前で FetchContent や find_package 等で導入済みの場合）は
#   取得処理を完全にスキップし、既存のターゲットをそのまま使う。
#   スキップしたかどうかは呼び出し元スコープに ${LIBRARY_NAME}_ALREADY_PROVIDED
#   としてセットされるので、後続の FetchContent_MakeAvailable / FetchContent_Populate を
#   同様に条件分岐で抑制すること。
#
# 注意:
#   FetchContent_MakeAvailable()は呼び出し側で実行してください
#   （${LIBRARY_NAME}_ALREADY_PROVIDED が TRUE の場合は呼び出さないこと）

function(add_external_package LIBRARY_NAME LOCAL_PATH)
    set(options "")
    set(oneValueArgs GIT_REPOSITORY GIT_TAG URL URL_HASH TARGET_CHECK SOURCE_SUBDIR)
    set(multiValueArgs "")
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(DEFINED ARG_TARGET_CHECK AND TARGET ${ARG_TARGET_CHECK})
        message(STATUS "${LIBRARY_NAME}: target '${ARG_TARGET_CHECK}' already provided by the including project - skip fetch")
        set(${LIBRARY_NAME}_ALREADY_PROVIDED TRUE PARENT_SCOPE)
        return()
    endif()
    set(${LIBRARY_NAME}_ALREADY_PROVIDED FALSE PARENT_SCOPE)

    set(_subdir_args "")
    if(DEFINED ARG_SOURCE_SUBDIR)
        set(_subdir_args SOURCE_SUBDIR ${ARG_SOURCE_SUBDIR})
    endif()

    set(FULL_LOCAL_PATH "${PROJECT_SOURCE_DIR}/${LOCAL_PATH}")
    if(EXISTS ${FULL_LOCAL_PATH})
        message(STATUS "Using local ${LIBRARY_NAME} from ${FULL_LOCAL_PATH}")
        FetchContent_Declare(${LIBRARY_NAME}
            SOURCE_DIR ${FULL_LOCAL_PATH}
            ${_subdir_args}
        )
    elseif(DEFINED ARG_URL)
        message(STATUS "Downloading ${LIBRARY_NAME} from ${ARG_URL}")
        if(DEFINED ARG_URL_HASH)
            FetchContent_Declare(${LIBRARY_NAME}
                URL ${ARG_URL}
                URL_HASH ${ARG_URL_HASH}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                ${_subdir_args}
            )
        else()
            FetchContent_Declare(${LIBRARY_NAME}
                URL ${ARG_URL}
                DOWNLOAD_EXTRACT_TIMESTAMP TRUE
                ${_subdir_args}
            )
        endif()
    elseif(DEFINED ARG_GIT_REPOSITORY)
        message(STATUS "Fetching ${LIBRARY_NAME} from ${ARG_GIT_REPOSITORY}")
        FetchContent_Declare(${LIBRARY_NAME}
            GIT_REPOSITORY ${ARG_GIT_REPOSITORY}
            GIT_TAG ${ARG_GIT_TAG}
            GIT_SHALLOW TRUE
            ${_subdir_args}
        )
    else()
        message(FATAL_ERROR "Either GIT_REPOSITORY or URL must be specified for ${LIBRARY_NAME}")
    endif()
endfunction()
