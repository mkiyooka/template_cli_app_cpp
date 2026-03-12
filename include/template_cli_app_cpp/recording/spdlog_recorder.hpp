#pragma once

#include <memory>

#include <spdlog/spdlog.h>

#include "template_cli_app_cpp/recording/data_recorder.hpp"

namespace recording {

/**
 * @brief spdlog を使った解析データレコーダー
 *
 * spdlog::logger を内包し、DataRecorder インターフェース経由でデータを出力する。
 * ログレベルではなく Enable/Disable で出力制御する。
 * パターンを "%v" にすることでメッセージのみを出力する（タイムスタンプ等なし）。
 *
 * spdlog::logger の生成とシンク設定は呼び出し側が行う。
 *
 * @code
 * auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("moduleX.csv");
 * auto inner = std::make_shared<spdlog::logger>("moduleX", sink);
 * SpdlogRecorder recorder(inner);
 * recorder.Enable();
 * recorder.Write("{},{:.6f}", step, value);
 * @endcode
 */
class SpdlogRecorder : public DataRecorder {
public:
    explicit SpdlogRecorder(std::shared_ptr<spdlog::logger> logger)
        : logger_(std::move(logger)) {
        logger_->set_pattern("%v");
        logger_->set_level(spdlog::level::off);
    }

    void Enable() override { logger_->set_level(spdlog::level::info); }

    void Disable() override { logger_->set_level(spdlog::level::off); }

    bool IsEnabled() const override { return logger_->level() != spdlog::level::off; }

    void Output(std::string_view msg) override { logger_->info(msg); }

    void Flush() override { logger_->flush(); }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

} // namespace recording
