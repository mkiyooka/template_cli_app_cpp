#define ANKERL_NANOBENCH_IMPLEMENT

#include <nanobench.h>

#include <filesystem>
#include <string>

#include <spdlog/async.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include "template_cli_app_cpp/logging/null_logger.hpp"
#include "template_cli_app_cpp/logging/spdlog_logger.hpp"
#include "support/spy_logger.hpp"

namespace {

constexpr const char *kSpdlogSyncFile = "/tmp/bench_spdlog_sync.log";
constexpr const char *kSpdlogAsyncFile = "/tmp/bench_spdlog_async.log";

// DI アダプター用ファイル
constexpr const char *kDiSpdlogSFile = "/tmp/bench_di_spdlog_sync.log";
constexpr const char *kDiSpdlogAFile = "/tmp/bench_di_spdlog_async.log";

constexpr int kBatchSize = 1000;

// Logger 経由でログを1件出力する（DI の呼び出し側を模擬）
void WriteOne(logging::Logger &logger, std::string_view msg) { logger.Log(logging::LogLevel::Info, msg); }

} // namespace

int main() {
    // ──────────────────────────────────────────────────────────────
    // spdlog セットアップ
    // ──────────────────────────────────────────────────────────────
    auto spdlog_sync = spdlog::basic_logger_mt("spdlog_sync", kSpdlogSyncFile);
    spdlog_sync->set_level(spdlog::level::info);
    spdlog_sync->set_pattern("%v");

    spdlog::init_thread_pool(8192, 1);
    auto spdlog_async = spdlog::basic_logger_mt<spdlog::async_factory>("spdlog_async", kSpdlogAsyncFile);
    spdlog_async->set_level(spdlog::level::info);
    spdlog_async->set_pattern("%v");

    spdlog_sync->info("warmup");
    spdlog_sync->flush();
    spdlog_async->info("warmup");
    spdlog_async->flush();

    // ──────────────────────────────────────────────────────────────
    // DI ロガーセットアップ
    // ──────────────────────────────────────────────────────────────
    auto di_spdlog_sync_inner = spdlog::basic_logger_mt("di_spdlog_sync", kDiSpdlogSFile);
    di_spdlog_sync_inner->set_pattern("%v");
    logging::SpdlogLogger di_spdlog_sync(di_spdlog_sync_inner);
    di_spdlog_sync.SetLevel(logging::LogLevel::Info);

    auto di_spdlog_async_inner = spdlog::basic_logger_mt<spdlog::async_factory>("di_spdlog_async", kDiSpdlogAFile);
    di_spdlog_async_inner->set_pattern("%v");
    logging::SpdlogLogger di_spdlog_async(di_spdlog_async_inner);
    di_spdlog_async.SetLevel(logging::LogLevel::Info);

    logging::NullLogger null_logger;
    SpyLogger test_logger;
    test_logger.SetLevel(logging::LogLevel::Info);

    // ──────────────────────────────────────────────────────────────
    // ベンチマーク
    // ──────────────────────────────────────────────────────────────
    ankerl::nanobench::Bench bench;
    bench.title("Logger Benchmark").unit("log call").warmup(200);

    // ════════════════════════════════════════════════════════════════
    // セクション1: レイテンシ — 直接呼び出し（ベースライン）
    //   sync : 呼び出しスレッドがI/O完了まで待機
    //   async: エンキュー + flush（バックエンド完了まで待機）
    // ════════════════════════════════════════════════════════════════

    bench.minEpochIterations(100000);
    bench.run("spdlog  sync  [latency:direct] 1 msg", [&] { spdlog_sync->info("Benchmark message {}", 42); });

    bench.minEpochIterations(1000);
    bench.run("spdlog  async [latency:direct] 1 msg (enqueue + flush)", [&] {
        spdlog_async->info("Benchmark message {}", 42);
        spdlog_async->flush();
    });

    // ════════════════════════════════════════════════════════════════
    // セクション2: レイテンシ — Logger 経由（DI）
    //   仮想関数呼び出しのオーバーヘッドを含む
    //   NullLogger / SpyLogger はI/O なしの純粋な仮想呼び出しコスト
    // ════════════════════════════════════════════════════════════════

    bench.minEpochIterations(100000);
    bench.run("NullLogger      [latency:DI   ] 1 msg (no-op)", [&] { WriteOne(null_logger, "Benchmark message 42"); });

    bench.minEpochIterations(1300000);
    bench.run("SpyLogger      [latency:DI   ] 1 msg (memory append)", [&] {
        WriteOne(test_logger, "Benchmark message 42");
    });

    bench.minEpochIterations(100000);
    bench.run("SpdlogLogger    [latency:DI   ] 1 msg (spdlog sync)", [&] {
        WriteOne(di_spdlog_sync, "Benchmark message 42");
    });

    bench.minEpochIterations(1000);
    bench.run("SpdlogLogger    [latency:DI   ] 1 msg (spdlog async + flush)", [&] {
        di_spdlog_async_inner->flush();
        WriteOne(di_spdlog_async, "Benchmark message 42");
        di_spdlog_async_inner->flush();
    });

    // ════════════════════════════════════════════════════════════════
    // セクション3: スループット — 直接呼び出し（バッチN件→flush）
    // ════════════════════════════════════════════════════════════════

    bench.batch(kBatchSize).minEpochIterations(100);
    bench.run("spdlog  sync  [throughput:direct] batch " + std::to_string(kBatchSize) + " msgs + flush", [&] {
        for (int i = 0; i < kBatchSize; ++i) {
            spdlog_sync->info("Batch message {}", i);
        }
        spdlog_sync->flush();
    });

    bench.minEpochIterations(3000);
    bench.run("spdlog  async [throughput:direct] batch " + std::to_string(kBatchSize) + " msgs + flush", [&] {
        for (int i = 0; i < kBatchSize; ++i) {
            spdlog_async->info("Batch message {}", i);
        }
        spdlog_async->flush();
    });

    // ════════════════════════════════════════════════════════════════
    // セクション4: スループット — Logger 経由（DI）
    // ════════════════════════════════════════════════════════════════

    bench.minEpochIterations(100);
    bench.run("NullLogger      [throughput:DI   ] batch " + std::to_string(kBatchSize) + " msgs (no-op)", [&] {
        for (int i = 0; i < kBatchSize; ++i) {
            WriteOne(null_logger, "Batch message");
        }
    });

    bench.run("SpyLogger      [throughput:DI   ] batch " + std::to_string(kBatchSize) + " msgs (memory)", [&] {
        test_logger.clear();
        for (int i = 0; i < kBatchSize; ++i) {
            WriteOne(test_logger, "Batch message");
        }
    });

    bench.minEpochIterations(100);
    bench.run("SpdlogLogger    [throughput:DI   ] batch " + std::to_string(kBatchSize) + " msgs (sync + flush)", [&] {
        for (int i = 0; i < kBatchSize; ++i) {
            WriteOne(di_spdlog_sync, "Batch message");
        }
        di_spdlog_sync_inner->flush();
    });

    bench.minEpochIterations(100);
    bench.run("SpdlogLogger    [throughput:DI   ] batch " + std::to_string(kBatchSize) + " msgs (async + flush)", [&] {
        for (int i = 0; i < kBatchSize; ++i) {
            WriteOne(di_spdlog_async, "Batch message");
        }
        di_spdlog_async_inner->flush();
    });

    spdlog::drop_all();

    for (const char *f : {kSpdlogSyncFile, kSpdlogAsyncFile, kDiSpdlogSFile, kDiSpdlogAFile}) {
        std::filesystem::remove(std::filesystem::path{f});
    }

    return 0;
}
