#include <template_cli_app_cpp/utility/csv_wrapper.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>

namespace {

// ──────────────────────────────────────────────────────
// サンプル用 CSV ファイル生成
// 列: id, category, value_a, value_b, flag
// flag=1 の割合: 約 10%
// ──────────────────────────────────────────────────────

constexpr std::array<const char *, 5> kCategories = {"A", "B", "C", "D", "E"};

std::filesystem::path GenerateSampleCsv(int num_rows) {
    const auto path = std::filesystem::temp_directory_path() / "example_csv_wrapper.csv";
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> value_dist(0.0, 1000.0);
    std::uniform_int_distribution<int> cat_dist(0, 4);
    std::bernoulli_distribution flag_dist(0.10);

    std::ofstream ofs(path);
    ofs << "id,category,value_a,value_b,flag\n";
    for (int i = 0; i < num_rows; ++i) {
        ofs << i << ','
            << kCategories[static_cast<size_t>(cat_dist(rng))] << ','
            << value_dist(rng) << ','
            << value_dist(rng) << ','
            << (flag_dist(rng) ? 1 : 0) << '\n';
    }
    return path;
}

} // namespace

int main() {
    constexpr int kNumRows = 1000;
    const auto path = GenerateSampleCsv(kNumRows);

    utility::CsvReader reader(path.string());

    // ── ReadFiltered: flag==1 の行から value_a, value_b を double で取得 ──
    auto predicate = [](const csv::CSVRow &row) {
        return row["flag"].get<int>() == 1;
    };

    const auto values = reader.ReadFiltered(predicate, {"value_a", "value_b"});

    std::cout << "[ReadFiltered]\n";
    if (values) {
        std::cout << "  filtered rows : " << values->size() / 2 << "\n";
        if (!values->empty()) {
            std::cout << "  first value_a : " << (*values)[0] << "\n";
            std::cout << "  first value_b : " << (*values)[1] << "\n";
        }
    } else {
        std::cerr << "  error: " << values.error() << "\n";
    }

    // ── ReadFilteredAsStrings: flag==1 の行から category を文字列で取得 ──
    const auto categories = reader.ReadFilteredAsStrings(predicate, {"category"});

    std::cout << "\n[ReadFilteredAsStrings]\n";
    if (categories) {
        std::cout << "  filtered rows  : " << categories->size() << "\n";
        if (!categories->empty()) {
            std::cout << "  first category : " << (*categories)[0] << "\n";
        }
    } else {
        std::cerr << "  error: " << categories.error() << "\n";
    }

    std::filesystem::remove(path);
    return 0;
}
