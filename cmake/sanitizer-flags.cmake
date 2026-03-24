# AddressSanitizer + UndefinedBehaviorSanitizer instrumentation flags
#
# add_compile_options / add_link_options を使わず INTERFACE ライブラリとして定義する。
# これにより FetchContent でビルドされるサードパーティには ASan フラグが伝播しない。
#
# 使い方:
#   target_link_libraries(<your_target> PRIVATE sanitizer_flags)

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

add_library(sanitizer_flags INTERFACE)

if(NOT ENABLE_SANITIZERS)
    return()
endif()

# clang / AppleClang / GCC すべて対応
if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
    message(WARNING "Sanitizers: clang or gcc required (current: ${CMAKE_CXX_COMPILER_ID}). Disabled.")
    return()
endif()

message(STATUS "Sanitizers   : ASan + UBSan enabled (${CMAKE_CXX_COMPILER_ID})")

target_compile_options(sanitizer_flags INTERFACE
    -fsanitize=address,undefined
    -fno-omit-frame-pointer
    -g
    -O1
)
target_link_options(sanitizer_flags INTERFACE -fsanitize=address,undefined)
