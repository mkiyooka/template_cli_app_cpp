#pragma once

#include <memory>
#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "template_cli_app_cpp/recording/data_recorder.hpp"
#include "template_cli_app_cpp/recording/null_recorder.hpp"
#include "template_cli_app_cpp/recording/spdlog_recorder.hpp"

namespace recording {

/**
 * @brief DataRecorder インスタンスを生成するファクトリ
 *
 * 出力先・フォーマットの選択を一箇所に集約し、呼び出し側が
 * spdlog の詳細を知らなくてもよくする。
 *
 * @code
 * // 汎用ファイルレコーダー
 * auto rec = RecorderFactory::MakeFile("moduleX", "out.txt");
 *
 * // CSV: ヘッダ行を自動出力し、以降は Write() で行を追記
 * auto csv = RecorderFactory::MakeCsvFile("results", "results.csv", "name,value,remainder");
 * csv->Enable();
 * csv->Write("{},{:.6f},{}", "doubled", 7.0, 1);
 *
 * // JSON Lines (NDJSON): 1行1JSONを追記
 * auto jl = RecorderFactory::MakeJsonLinesFile("results", "results.jsonl");
 * jl->Enable();
 * jl->Write("{}", builder.Serialize(false));
 * @endcode
 */
struct RecorderFactory {
    /**
     * @brief ファイルに書き込む汎用同期レコーダーを生成する
     *
     * spdlog のパターンを "%v"（メッセージのみ）に設定する。
     * 初期状態は disabled。
     *
     * @param name      spdlog 内部名（重複不可）
     * @param file_path 出力ファイルパス
     */
    static std::unique_ptr<DataRecorder> MakeFile(const std::string &name, const std::string &file_path) {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path);
        auto inner = std::make_shared<spdlog::logger>(name, sink);
        return std::make_unique<SpdlogRecorder>(inner);
    }

    /**
     * @brief CSV ファイルに書き込む同期レコーダーを生成する
     *
     * ファクトリ生成時にヘッダ行を即時書き込む。
     * 以降は Write() で 1 行ずつデータ行を追記する。
     * 初期状態は disabled（Enable() 後に Write() すること）。
     *
     * @param name      spdlog 内部名（重複不可）
     * @param file_path 出力ファイルパス（例: "results.csv"）
     * @param header    CSVヘッダ行（例: "step,value,label"）
     */
    static std::unique_ptr<DataRecorder>
    MakeCsvFile(const std::string &name, const std::string &file_path, const std::string &header) {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, /* truncate = */ true);
        auto inner = std::make_shared<spdlog::logger>(name, sink);
        // ヘッダ行をファクトリ内で書き込む（Enable 不要）
        inner->set_pattern("%v");
        inner->set_level(spdlog::level::info);
        inner->info(header);
        inner->flush();
        return std::make_unique<SpdlogRecorder>(inner);
    }

    /**
     * @brief JSON Lines (NDJSON) ファイルに書き込む同期レコーダーを生成する
     *
     * 1行1JSONを追記する形式。Write() に JSON 文字列を渡す。
     * MakeFile() と同じ動作だが、用途を名前で明示する。
     * 初期状態は disabled。
     *
     * @param name      spdlog 内部名（重複不可）
     * @param file_path 出力ファイルパス（例: "results.jsonl"）
     */
    static std::unique_ptr<DataRecorder> MakeJsonLinesFile(const std::string &name, const std::string &file_path) {
        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path);
        auto inner = std::make_shared<spdlog::logger>(name, sink);
        return std::make_unique<SpdlogRecorder>(inner);
    }

    /**
     * @brief 標準出力（カラー付き）に書き込む同期レコーダーを生成する
     *
     * @param name spdlog 内部名（重複不可）
     */
    static std::unique_ptr<DataRecorder> MakeConsole(const std::string &name) {
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto inner = std::make_shared<spdlog::logger>(name, sink);
        return std::make_unique<SpdlogRecorder>(inner);
    }

    /**
     * @brief 何も出力しないレコーダーを生成する（テスト・無効化用）
     */
    static std::unique_ptr<DataRecorder> MakeNull() { return std::make_unique<NullRecorder>(); }
};

} // namespace recording
