#define ANKERL_NANOBENCH_IMPLEMENT

#include <nanobench.h>

#include <csv.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr int kNumRows = 2'000'000;
constexpr uint64_t kRandomSeed = 12345;

constexpr std::array<const char *, 5> kCategories = {"A", "B", "C", "D", "E"};

// ──────────────────────────────────────────────────────────────
// 5列 CSV 生成
// 列: id, category, value_a, value_b, flag
// flag=1 の割合: 約 10%
// ──────────────────────────────────────────────────────────────

std::filesystem::path GenerateCsvFile5Col(int num_rows) {
    const auto path = std::filesystem::temp_directory_path() / "bench_csv_5col.csv";

    // 再現可能なデータにするため固定シードを使う
    std::mt19937_64 rng(kRandomSeed); // NOLINT(cert-msc32-c,cert-msc51-cpp)
    std::uniform_real_distribution<double> value_dist(0.0, 1000.0);
    std::uniform_int_distribution<int> cat_dist(0, 4);
    std::bernoulli_distribution flag_dist(0.10);

    std::ofstream ofs(path);
    ofs << "id,category,value_a,value_b,flag\n";
    for (int i = 0; i < num_rows; ++i) {
        ofs << i << ',' << kCategories.at(static_cast<size_t>(cat_dist(rng))) << ',' << value_dist(rng) << ','
            << value_dist(rng) << ',' << (flag_dist(rng) ? 1 : 0) << '\n';
    }
    return path;
}

// ──────────────────────────────────────────────────────────────
// 30列 CSV 生成
// 列: id, category, val00〜val25(double×26), value_a, value_b, flag
// 合計 30列。double は整数部2桁に制限してファイルサイズを抑える
// ──────────────────────────────────────────────────────────────

std::filesystem::path GenerateCsvFile30Col(int num_rows) {
    const auto path = std::filesystem::temp_directory_path() / "bench_csv_30col.csv";

    // 再現可能なデータにするため固定シードを使う
    std::mt19937_64 rng(kRandomSeed); // NOLINT(cert-msc32-c,cert-msc51-cpp)
    std::uniform_real_distribution<double> value_dist(0.0, 99.99);
    std::uniform_int_distribution<int> cat_dist(0, 4);
    std::bernoulli_distribution flag_dist(0.10);

    std::ofstream ofs(path);
    ofs << "id,category";
    for (int c = 0; c < 26; ++c) {
        ofs << ",val" << (c < 10 ? "0" : "") << c;
    }
    ofs << ",value_a,value_b,flag\n";  // col 28, 29, 30 → 合計 31... 調整:

    // 実際の列インデックス（0-based）:
    //   0:id  1:category  2-27:val00-val25  28:value_a  29:value_b  30:flag
    // 計 31列だが「30列程度」の意図に合わせて実装する

    for (int i = 0; i < num_rows; ++i) {
        ofs << i << ',' << kCategories.at(static_cast<size_t>(cat_dist(rng)));
        for (int c = 0; c < 26; ++c) {
            ofs << ',' << value_dist(rng);
        }
        ofs << ',' << value_dist(rng)   // value_a
            << ',' << value_dist(rng)   // value_b
            << ',' << (flag_dist(rng) ? 1 : 0) << '\n';
    }
    return path;
}

// ──────────────────────────────────────────────────────────────
// フィルタ通過行数をプリスキャンで確定
// ──────────────────────────────────────────────────────────────

int64_t CountFiltered(const std::string &path, int flag_idx) {
    int64_t count = 0;
    csv::CSVReader reader(path);
    for (auto &row : reader) {
        if (row[flag_idx].get<int>() == 1) {
            ++count;
        }
    }
    return count;
}

} // namespace

// ──────────────────────────────────────────────────────────────
// 補助関数群
// ──────────────────────────────────────────────────────────────

namespace csv_bench {

// (A) 列名アクセス: ループ内で row["name"] を使う素直な実装
//     毎行、列名→インデックスのハッシュ探索が発生する
std::vector<double> ReadFilteredByName(
    const std::string &path, const std::function<bool(const csv::CSVRow &)> &predicate,
    const std::vector<std::string> &output_cols
) {
    std::vector<double> result;
    csv::CSVReader reader(path);
    for (auto &row : reader) {
        if (predicate(row)) {
            for (const auto &col : output_cols) {
                result.push_back(row[col].get<double>());
            }
        }
    }
    return result;
}

// (B) インデックスアクセス: 事前に index_of() で列番号を解決し、直接アクセス
//     ループ内のハッシュ探索コストを排除
std::vector<double> ReadFilteredByIndex(
    const std::string &path, const std::function<bool(const csv::CSVRow &)> &predicate,
    const std::vector<int> &output_col_indices
) {
    std::vector<double> result;
    csv::CSVReader reader(path);
    for (auto &row : reader) {
        if (predicate(row)) {
            for (const int idx : output_col_indices) {
                result.push_back(row[idx].get<double>());
            }
        }
    }
    return result;
}

// (C) string_view: 型変換コストを最小化
//     インデックスアクセス + get<string_view>() で double 変換を回避
//     型変換が占めるコストを (B) との比較で分離計測できる
std::vector<std::string> ReadFilteredStringView(
    const std::string &path, const std::function<bool(const csv::CSVRow &)> &predicate,
    const std::vector<int> &output_col_indices
) {
    std::vector<std::string> result;
    csv::CSVReader reader(path);
    for (auto &row : reader) {
        if (predicate(row)) {
            for (const int idx : output_col_indices) {
                // string_view はイテレータ進行後に無効化されるため string にコピー
                result.emplace_back(row[idx].get<csv::string_view>());
            }
        }
    }
    return result;
}

// (D) ハードコード: std::function / std::vector<int> の間接コストを排除
//     フィルタ条件・出力列を直接記述し、コンパイラの完全インライン化を促す
//     initializer_list は定数サイズに展開されるため間接コストが極小
std::vector<double>
ReadFilteredHardcoded(const std::string &path, int flag_idx, std::initializer_list<int> out_indices) {
    std::vector<double> result;
    csv::CSVReader reader(path);
    for (auto &row : reader) {
        if (row[flag_idx].get<int>() == 1) {
            for (const int idx : out_indices) {
                result.push_back(row[idx].get<double>());
            }
        }
    }
    return result;
}

} // namespace csv_bench

// ──────────────────────────────────────────────────────────────
// セクション: 素読みスループット
// ──────────────────────────────────────────────────────────────

void BenchRawRead(ankerl::nanobench::Bench &bench,
                  const std::string &path,
                  int num_rows,
                  const char *label) {
    // minEpochTime: 1ケースあたり最大2秒で打ち切り
    bench.batch(num_rows).minEpochIterations(3).minEpochTime(std::chrono::milliseconds(500));

    bench.run(std::string("csv-parser ") + label + " [raw] default chunk (10MB)", [&] {
        csv::CSVReader reader(path);
        int64_t count = 0;
        for (auto &row : reader) {
            ankerl::nanobench::doNotOptimizeAway(row);
            ++count;
        }
        ankerl::nanobench::doNotOptimizeAway(count);
    });

    bench.run(std::string("csv-parser ") + label + " [raw] chunk 64MB", [&] {
        csv::CSVFormat fmt;
        fmt.chunk_size(std::size_t{64} * 1024 * 1024);
        csv::CSVReader reader(path, fmt);
        int64_t count = 0;
        for (auto &row : reader) {
            ankerl::nanobench::doNotOptimizeAway(row);
            ++count;
        }
        ankerl::nanobench::doNotOptimizeAway(count);
    });
}

// ──────────────────────────────────────────────────────────────
// セクション: フィルタ付き読み込み（flag==1、約 10% 通過）
// ──────────────────────────────────────────────────────────────

void BenchFiltered(ankerl::nanobench::Bench &bench,
                   const std::string &path,
                   int num_rows,
                   const char *label,
                   const std::vector<std::string> &out_col_names,
                   const std::vector<int> &out_col_indices,
                   int flag_idx) {
    (void)num_rows;

    // フィルタ通過行数をプリスキャンで確定（batch 設定に使用）
    const int64_t filtered_count = CountFiltered(path, flag_idx);

    // (A) 列名アクセス
    {
        auto pred = [&flag_idx](const csv::CSVRow &row) {
            return row[flag_idx].get<int>() == 1;
        };
        bench.batch(filtered_count).minEpochIterations(3).minEpochTime(std::chrono::milliseconds(500));
        bench.run(std::string("csv-parser ") + label + " [filtered:A] by name", [&] {
            auto result = csv_bench::ReadFilteredByName(path, pred, out_col_names);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // (B) インデックスアクセス
    {
        auto pred = [&flag_idx](const csv::CSVRow &row) {
            return row[flag_idx].get<int>() == 1;
        };
        bench.batch(filtered_count).minEpochIterations(3).minEpochTime(std::chrono::milliseconds(500));
        bench.run(std::string("csv-parser ") + label + " [filtered:B] by index", [&] {
            auto result = csv_bench::ReadFilteredByIndex(path, pred, out_col_indices);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // (C) string_view（double 変換なし）
    {
        auto pred = [&flag_idx](const csv::CSVRow &row) {
            return row[flag_idx].get<int>() == 1;
        };
        bench.batch(filtered_count).minEpochIterations(3).minEpochTime(std::chrono::milliseconds(500));
        bench.run(std::string("csv-parser ") + label + " [filtered:C] string_view", [&] {
            auto result = csv_bench::ReadFilteredStringView(path, pred, out_col_indices);
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }

    // (D) ハードコード（std::function / std::vector<int> なし）
    {
        bench.batch(filtered_count).minEpochIterations(3).minEpochTime(std::chrono::milliseconds(500));
        bench.run(std::string("csv-parser ") + label + " [filtered:D] hardcoded", [&] {
            std::vector<double> result;
            csv::CSVReader reader(path);
            for (auto &row : reader) {
                if (row[flag_idx].get<int>() == 1) {
                    for (const int idx : out_col_indices) {
                        result.push_back(row[idx].get<double>());
                    }
                }
            }
            ankerl::nanobench::doNotOptimizeAway(result);
        });
    }
}

namespace {

int Run() {
    // 5列版: kNumRows の 1/10、31列版: kNumRows の 1/100
    constexpr int kNumRows5col  = kNumRows / 10;
    constexpr int kNumRows31col = kNumRows / 100;

    // ── CSV ファイル生成 ──
    const auto path5  = GenerateCsvFile5Col(kNumRows5col);
    const auto path30 = GenerateCsvFile30Col(kNumRows31col);

    ankerl::nanobench::Bench bench;
    bench.title("csv-parser Read Benchmark").unit("row");

    // ════════════════════════════════════════════════════════════════
    // 5列 CSV
    // ════════════════════════════════════════════════════════════════

    // 素読み
    BenchRawRead(bench, path5.string(), kNumRows5col, "[5col ]");

    // フィルタ付き
    // col: id(0) category(1) value_a(2) value_b(3) flag(4)
    BenchFiltered(bench, path5.string(), kNumRows5col, "[5col ]",
                  {"value_a", "value_b"}, {2, 3}, /*flag_idx=*/4);

    // ════════════════════════════════════════════════════════════════
    // 31列 CSV (id, category, val00-val25, value_a, value_b, flag)
    // col: id(0) category(1) val00-val25(2-27) value_a(28) value_b(29) flag(30)
    // ════════════════════════════════════════════════════════════════

    // 素読み
    BenchRawRead(bench, path30.string(), kNumRows31col, "[31col]");

    // フィルタ付き
    BenchFiltered(bench, path30.string(), kNumRows31col, "[31col]",
                  {"value_a", "value_b"}, {28, 29}, /*flag_idx=*/30);

    // ── 後片付け ──
    std::filesystem::remove(path5);
    std::filesystem::remove(path30);

    return 0;
}

} // namespace

int main() {
    try {
        return Run();
    } catch (const std::exception &e) {
        std::cerr << "error: " << e.what() << '\n';
    } catch (...) {
        std::cerr << "error: unknown exception\n";
    }
    return EXIT_FAILURE;
}
