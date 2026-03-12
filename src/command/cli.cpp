#include <cstdlib>
#include <memory>
#include <vector>

#include <CLI/CLI.hpp>
#include <fmt/base.h>
#include <fmt/format.h>

#include "command/subcommand.hpp"
#include "config/config_manager.hpp"
#include "config/config_schema.hpp"
#include "template_cli_app_cpp/logging/logger_factory.hpp"
#include "template_cli_app_cpp/output/output_context.hpp"
#include "template_cli_app_cpp/recording/recorder_factory.hpp"
#include "template_cli_app_cpp/recording/recorder_manager.hpp"
#include "template_cli_app_cpp/utility/yyjson_wrapper.hpp"

namespace {

enum class OutputModule : std::uint8_t { kResultsCsv, kResultsJson };

// 設定内容をターミナルに表示する（デバッグ・確認用）
void ShowConfig(const Config &conf) {
    std::apply(
        [&](auto &&...field) { ([&] { fmt::print("{}: {}\n", field.config_key, conf.*field.member); }(), ...); },
        config::kConfigSchema
    );
    for (const auto &p : conf.plugins) {
        fmt::print("plugin: file={}, number={}\n", p.file, p.number);
    }
    for (std::size_t i = 0; i < kSubcommandMappingCount; ++i) {
        const auto &m = kSubcommandMappings[i];
        fmt::print("subcommands.{}: a={}, b={}\n", m.key, (conf.*m.member).a, (conf.*m.member).b);
    }
}

// Logger と DataRecorder を使った出力サンプル
//
// Logger のフォーマット:
//   MakeConsole() の pattern 引数で spdlog のパターン文字列を指定する。
//   空文字列（デフォルト）の場合は spdlog 標準書式が使われる。
//   パターン記号: %Y=年 %m=月 %d=日 %H=時 %M=分 %S=秒 %e=ミリ秒
//                 %n=ロガー名 %l=レベル(小文字) %L=レベル(1文字) %v=メッセージ
//
// DataRecorder のフォーマット:
//   - MakeCsvFile()      : ヘッダ行を自動出力。Write() で行を追記する。
//   - MakeJsonLinesFile(): 1行1JSON (NDJSON)。Write() に JSON 文字列を渡す。
void RunOutputSample(output::OutputContext<OutputModule> &output_context) {
    logging::Logger &logger = output_context.GetLogger();
    recording::DataRecorder &csv_recorder = output_context.GetRecorders()[OutputModule::kResultsCsv];
    recording::DataRecorder &json_recorder = output_context.GetRecorders()[OutputModule::kResultsJson];

    // --- 入力パラメータ ---
    constexpr double kInput = 3.5;
    constexpr int kStart = 1;
    constexpr int kCount = 5;
    constexpr int kDivisor = 7;
    constexpr int kTargetValue = 15;

    // 入力変数をログに出力
    logger.Log(logging::LogLevel::Info, "=== output sample start ===");
    logger.Log(
        logging::LogLevel::Info,
        fmt::format(
            "input={}, start={}, count={}, divisor={}, target_value={}", kInput, kStart, kCount, kDivisor, kTargetValue
        )
    );

    // --- 計算 ---
    constexpr double kDoubled = kInput * 2.0;
    std::vector<int> sequence;
    sequence.reserve(kCount);
    for (int i = 0; i < kCount; ++i) {
        sequence.push_back(kStart + i);
    }
    const int remainder = kTargetValue % kDivisor;

    logger.Log(logging::LogLevel::Debug, fmt::format("doubled({}) = {}", kInput, kDoubled));
    logger.Log(logging::LogLevel::Debug, fmt::format("sequence({}, {}) size={}", kStart, kCount, sequence.size()));
    logger.Log(logging::LogLevel::Debug, fmt::format("remainder({}) = {}", kTargetValue, remainder));

    // --- CSV 出力（ヘッダはファクトリ生成時に書き込み済み）---
    csv_recorder.Enable();
    csv_recorder.Write("{},{:.6f},{}", "doubled", kDoubled, remainder);
    csv_recorder.Write("{},{:.6f},{}", "single", kInput, 2);
    csv_recorder.Flush();
    csv_recorder.Disable();

    // --- JSON Lines 出力（1行1JSON / NDJSON）---
    json_recorder.Enable();

    utility::JsonBuilder builder;

    // inputs サブオブジェクト
    auto inputs_object = builder.AddNested("inputs");
    builder.AddToNested(inputs_object, "input", kInput);
    builder.AddToNested(inputs_object, "start", kStart);
    builder.AddToNested(inputs_object, "count", kCount);
    builder.AddToNested(inputs_object, "divisor", kDivisor);
    builder.AddToNested(inputs_object, "target_value", kTargetValue);

    // results サブオブジェクト（sequence は vector<int> として Add() を使用）
    auto results_object = builder.AddNested("results");
    builder.AddToNested(results_object, "doubled", kDoubled);
    builder.AddToNested(results_object, "remainder", remainder);
    // sequence は vector<int> なのでトップレベルの Add() で追加
    builder.Add("sequence", sequence);

    json_recorder.Write("{}", builder.Serialize(/* pretty = */ false));
    json_recorder.Flush();
    json_recorder.Disable();

    logger.Log(logging::LogLevel::Info, "=== output sample end ===");
}

} // namespace

int RunCli(int argc, char *argv[]) {
    CLI::App app{"Command line parser demonstration with different subcommand styles"};
    argv = app.ensure_utf8(argv);

    std::string config_file;
    app.add_option("-c,--config", config_file, "Configuration file");

    config::ConfigManager config_manager;
    config_manager.RegisterOptions(app);

    Config config; // CLI サブコマンドオプションのバインド先

    // callback方式のサブコマンド (add, subtract)
    SetCallbackSubcommands(app, config);

    // got_subcommand方式のサブコマンド (multiply, divide)
    SetGotSubcommands(app, config);

    try {
        app.parse(argc, argv);
    } catch (const CLI::CallForHelp &e) {
        std::exit(app.exit(e)); // app.exit(e) prints help
    }

    // got_subcommand方式のサブコマンド実行
    ExecuteGotSubcommands(app, config);

    // スキーマフィールドを解決（CLI引数 > 設定ファイル > デフォルト値）
    const Config resolved = config_manager.Resolve(config_file);
    config.title = resolved.title;
    config.value = resolved.value;

    // スキーマ外フィールドはファイルの生値から取得
    const Config &file_vals = config_manager.GetFileValues();
    config.plugins = file_vals.plugins;

    // サブコマンドが CLI から指定されていれば config の値を優先、未指定ならファイル値を使う
    for (std::size_t i = 0; i < kSubcommandMappingCount; ++i) {
        const auto &m = kSubcommandMappings[i];
        if (!app.got_subcommand(m.key)) {
            config.*m.member = file_vals.*m.member;
        }
    }

    ShowConfig(config);

    // 出力サンプル: Logger / DataRecorder のフォーマット指定例
    //
    // Logger: pattern 引数でタイムスタンプ・ロガー名・レベルを含む書式を指定する。
    //   省略時は spdlog のデフォルト書式（"[timestamp][name][level]message"）。
    auto logger = logging::LoggerFactory::MakeConsole("app", logging::LogLevel::Debug, "[%Y-%m-%d %H:%M:%S.%e][%n][%^%l%$]%v");

    // DataRecorder: CSV と JSON Lines (NDJSON) の 2 形式を使い分ける例。
    //   MakeCsvFile()      - ヘッダ行を生成時に書き込み、Write() で行を追記。
    //   MakeJsonLinesFile() - Write() に JSON 文字列を渡して 1 行 1 JSON で追記。
    recording::RecorderManager<OutputModule> recorder_manager;
    recorder_manager.RegisterRecorder(
        OutputModule::kResultsCsv,
        recording::RecorderFactory::MakeCsvFile("results_csv", "output/results.csv", "name,value,remainder")
    );
    recorder_manager.RegisterRecorder(
        OutputModule::kResultsJson,
        recording::RecorderFactory::MakeJsonLinesFile("results_json", "output/results.jsonl")
    );
    output::OutputContext<OutputModule> output_context(*logger, recorder_manager);
    RunOutputSample(output_context);

    return 0;
}
