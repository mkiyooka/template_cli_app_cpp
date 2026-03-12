#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "support/temp_file.hpp"
#include "template_cli_app_cpp/utility/csv_wrapper.hpp"

static const std::string kTestCsv =
    "flag,value,label\n"
    "1,1.5,alpha\n"
    "0,2.0,beta\n"
    "1,3.5,gamma\n";

// ──────────────────────────────────────────────────────────────
// ReadFiltered (return 版)
// ──────────────────────────────────────────────────────────────

TEST_CASE("CsvReader: ReadFiltered return版 正常系") {
    const TempFile tmp("test_csv_readfiltered.csv", kTestCsv);
    utility::CsvReader reader(tmp.Str());

    auto result = reader.ReadFiltered(
        [](const csv::CSVRow &row) { return row["flag"].get<int>() == 1; },
        {"value"});

    REQUIRE(result.has_value());
    const auto &vals = result.value();
    REQUIRE(vals.size() == 2);
    CHECK(vals[0] == doctest::Approx(1.5));
    CHECK(vals[1] == doctest::Approx(3.5));
}

// ──────────────────────────────────────────────────────────────
// ReadFilteredAsStrings (return 版)
// ──────────────────────────────────────────────────────────────

TEST_CASE("CsvReader: ReadFilteredAsStrings return版 正常系") {
    const TempFile tmp("test_csv_readfilteredasstrings.csv", kTestCsv);
    utility::CsvReader reader(tmp.Str());

    auto result = reader.ReadFilteredAsStrings(
        [](const csv::CSVRow &row) { return row["flag"].get<int>() == 1; },
        {"label"});

    REQUIRE(result.has_value());
    const auto &vals = result.value();
    REQUIRE(vals.size() == 2);
    CHECK(vals[0] == "alpha");
    CHECK(vals[1] == "gamma");
}

// ──────────────────────────────────────────────────────────────
// ReadFiltered (out-param 版) — 事前データのクリア確認
// ──────────────────────────────────────────────────────────────

TEST_CASE("CsvReader: ReadFiltered out-param版 正常系 / 事前データのクリア確認") {
    const TempFile tmp("test_csv_outparam.csv", kTestCsv);
    utility::CsvReader reader(tmp.Str());

    // 事前に別データを詰めた vector を渡す
    std::vector<double> out = {999.0, 888.0, 777.0};

    auto result = reader.ReadFiltered(
        [](const csv::CSVRow &row) { return row["flag"].get<int>() == 1; },
        {"value"},
        out);

    REQUIRE(result.has_value());
    // 事前データは消えて、フィルタ結果だけが入っていること
    REQUIRE(out.size() == 2);
    CHECK(out[0] == doctest::Approx(1.5));
    CHECK(out[1] == doctest::Approx(3.5));
}

// ──────────────────────────────────────────────────────────────
// エラー系: 存在しない列名
// ──────────────────────────────────────────────────────────────

TEST_CASE("CsvReader: 存在しない列名でエラーが返る") {
    const TempFile tmp("test_csv_error.csv", kTestCsv);
    utility::CsvReader reader(tmp.Str());

    SUBCASE("ReadFiltered return版") {
        auto result = reader.ReadFiltered(
            [](const csv::CSVRow &) { return true; },
            {"nonexistent_col"});

        CHECK_FALSE(result.has_value());
        CHECK(result.error().find("column not found") != std::string::npos);
    }

    SUBCASE("ReadFilteredAsStrings return版") {
        auto result = reader.ReadFilteredAsStrings(
            [](const csv::CSVRow &) { return true; },
            {"nonexistent_col"});

        CHECK_FALSE(result.has_value());
        CHECK(result.error().find("column not found") != std::string::npos);
    }

    SUBCASE("ReadFiltered out-param版") {
        std::vector<double> out;
        auto result = reader.ReadFiltered(
            [](const csv::CSVRow &) { return true; },
            {"nonexistent_col"},
            out);

        CHECK_FALSE(result.has_value());
        CHECK(result.error().find("column not found") != std::string::npos);
    }
}
