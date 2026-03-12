#define ANKERL_NANOBENCH_IMPLEMENT

#include <nanobench.h>

#include <nlohmann/json.hpp>
#include <yyjson.h>

#include "template_cli_app_cpp/utility/yyjson_wrapper.hpp"

namespace {

// ──────────────────────────────────────────────────────────────
// 計測シナリオ: フラットオブジェクト（5フィールド）
// ──────────────────────────────────────────────────────────────

void BenchYyjsonFlat(ankerl::nanobench::Bench &bench) {
    bench.run("yyjson_wrapper  [flat] 5 fields + serialize", [&] {
        utility::JsonBuilder builder;
        builder.Add("name", "Alice");
        builder.Add("age", 30);
        builder.Add("score", 98.6);
        builder.Add("active", true);
        builder.Add("id", 12345);
        std::string s = builder.Serialize();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

void BenchNlohmannFlat(ankerl::nanobench::Bench &bench) {
    bench.run("nlohmann::json  [flat] 5 fields + dump", [&] {
        nlohmann::json j;
        j["name"] = "Alice";
        j["age"] = 30;
        j["score"] = 98.6;
        j["active"] = true;
        j["id"] = 12345;
        std::string s = j.dump();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

// ──────────────────────────────────────────────────────────────
// 計測シナリオ: ネストオブジェクト
// ──────────────────────────────────────────────────────────────

void BenchYyjsonNested(ankerl::nanobench::Bench &bench) {
    bench.run("yyjson_wrapper  [nested] 2-level + serialize", [&] {
        utility::JsonBuilder builder;
        auto inputs = builder.AddNested("inputs");
        builder.AddToNested(inputs, "x", 3.5);
        builder.AddToNested(inputs, "n", 5);
        auto results = builder.AddNested("results");
        builder.AddToNested(results, "doubled", 7.0);
        builder.AddToNested(results, "remainder", 3);
        std::string s = builder.Serialize();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

void BenchNlohmannNested(ankerl::nanobench::Bench &bench) {
    bench.run("nlohmann::json  [nested] 2-level + dump", [&] {
        nlohmann::json j;
        j["inputs"]["x"] = 3.5;
        j["inputs"]["n"] = 5;
        j["results"]["doubled"] = 7.0;
        j["results"]["remainder"] = 3;
        std::string s = j.dump();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

// ──────────────────────────────────────────────────────────────
// 計測シナリオ: 整数配列（10要素）
// ──────────────────────────────────────────────────────────────

void BenchYyjsonArray(ankerl::nanobench::Bench &bench) {
    const std::vector<int> seq = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    bench.run("yyjson_wrapper  [array] int[10] + serialize", [&] {
        utility::JsonBuilder builder;
        builder.Add("seq", seq);
        std::string s = builder.Serialize();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

void BenchNlohmannArray(ankerl::nanobench::Bench &bench) {
    const std::vector<int> seq = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    bench.run("nlohmann::json  [array] int[10] + dump", [&] {
        nlohmann::json j;
        j["seq"] = seq;
        std::string s = j.dump();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

// ──────────────────────────────────────────────────────────────
// 計測シナリオ: 複合（フラット + ネスト + 配列）
// ──────────────────────────────────────────────────────────────

void BenchYyjsonComplex(ankerl::nanobench::Bench &bench) {
    const std::vector<int> seq = {1, 2, 3, 4, 5};
    const std::vector<std::string> tags = {"alpha", "beta", "gamma"};
    bench.run("yyjson_wrapper  [complex] flat+nested+array + serialize", [&] {
        utility::JsonBuilder builder;
        builder.Add("name", "demo");
        builder.Add("version", 1);
        auto meta = builder.AddNested("meta");
        builder.AddToNested(meta, "author", "test");
        builder.AddToNested(meta, "debug", false);
        builder.Add("values", seq);
        builder.Add("tags", tags);
        std::string s = builder.Serialize();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

void BenchNlohmannComplex(ankerl::nanobench::Bench &bench) {
    const std::vector<int> seq = {1, 2, 3, 4, 5};
    const std::vector<std::string> tags = {"alpha", "beta", "gamma"};
    bench.run("nlohmann::json  [complex] flat+nested+array + dump", [&] {
        nlohmann::json j;
        j["name"] = "demo";
        j["version"] = 1;
        j["meta"]["author"] = "test";
        j["meta"]["debug"] = false;
        j["values"] = seq;
        j["tags"] = tags;
        std::string s = j.dump();
        ankerl::nanobench::doNotOptimizeAway(s);
    });
}

} // namespace

int main() {
    ankerl::nanobench::Bench bench;
    bench.title("JSON Build + Serialize Benchmark").unit("op").warmup(500).minEpochIterations(50000);

    // ── フラットオブジェクト ──
    BenchYyjsonFlat(bench);
    BenchNlohmannFlat(bench);

    // ── ネストオブジェクト ──
    BenchYyjsonNested(bench);
    BenchNlohmannNested(bench);

    // ── 配列 ──
    BenchYyjsonArray(bench);
    BenchNlohmannArray(bench);

    // ── 複合 ──
    BenchYyjsonComplex(bench);
    BenchNlohmannComplex(bench);

    return 0;
}
