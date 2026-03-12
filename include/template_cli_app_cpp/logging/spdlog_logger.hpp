#pragma once

#include <memory>

#include <spdlog/spdlog.h>

#include "template_cli_app_cpp/logging/logger.hpp"

namespace logging {

/**
 * @brief spdlog を使ったロガー
 *
 * spdlog の同期・非同期ロガーを Logger インターフェース経由で利用する。
 * spdlog::logger の生成は呼び出し側が行う。
 *
 * @code
 * auto sl = spdlog::basic_logger_mt("app", "app.log");
 * SpdlogLogger logger(sl);
 * process(logger);
 * @endcode
 */
class SpdlogLogger : public Logger {
public:
    explicit SpdlogLogger(std::shared_ptr<spdlog::logger> logger)
        : logger_(std::move(logger)) {}

    void Log(LogLevel lvl, std::string_view msg) override { logger_->log(ToSpdlogLevel(lvl), msg); }

    void SetLevel(LogLevel lvl) override {
        level_ = lvl;
        logger_->set_level(ToSpdlogLevel(lvl));
    }

    LogLevel Level() const override { return level_; }

private:
    std::shared_ptr<spdlog::logger> logger_;
    LogLevel level_ = LogLevel::Trace;

    static spdlog::level::level_enum ToSpdlogLevel(LogLevel lvl) {
        switch (lvl) {
            case LogLevel::Trace:
                return spdlog::level::trace;
            case LogLevel::Debug:
                return spdlog::level::debug;
            case LogLevel::Info:
                return spdlog::level::info;
            case LogLevel::Warn:
                return spdlog::level::warn;
            case LogLevel::Error:
                return spdlog::level::err;
            case LogLevel::Critical:
                return spdlog::level::critical;
            default:
                return spdlog::level::off;
        }
    }
};

} // namespace logging
