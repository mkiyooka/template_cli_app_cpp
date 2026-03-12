#pragma once

#include "template_cli_app_cpp/recording/data_recorder.hpp"

namespace recording {

/**
 * @brief 何も出力しないレコーダー
 *
 * 出力を完全に無効化したい場合や、DI先のデフォルト実装として使用する。
 */
class NullRecorder : public DataRecorder {
public:
    void Enable() override {}
    void Disable() override {}
    bool IsEnabled() const override { return false; }
    void Output(std::string_view /*message*/) override {}
    void Flush() override {}
};

} // namespace recording
