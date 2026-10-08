#pragma once

// このファイルは doctest の記述パターンをサンプルとして示す。
// テスト対象は外部ライブラリに依存せず、ファイル内でインライン定義する。
//
// 使用方法:
//   1. テストファイルに #include "support/test_doctest_usage.hpp" を追加する
//   2. DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN を定義したファイルで include する

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

#include <doctest/doctest.h>

// ─── テスト対象のインライン定義 ──────────────────────────────────────────────

// ヘッダ内の無名名前空間は include した翻訳単位ごとに別実体を作るため使わない（cert-dcl59-cpp）。
// 名前付き名前空間 + inline 関数にする。
namespace doctest_sample {

// 2倍にする関数（浮動小数点比較サンプル用）
inline double DoubleIt(double x) { return x * 2.0; }

// start から count 個の連続整数を返す（コンテナ比較サンプル用）
inline std::vector<int> MakeRange(int start, int count) {
    std::vector<int> result(count);
    std::iota(result.begin(), result.end(), start);
    return result;
}

// 剰余を計算するクラス（クラステスト・Fixture サンプル用）
class Modulo {
public:
    explicit Modulo(int divisor) : divisor_(divisor) {}
    int Remainder(int value) const { return value % divisor_; }

private:
    int divisor_;
};

} // namespace doctest_sample

// ─── サンプル: 浮動小数点比較（doctest::Approx） ─────────────────────────────

TEST_CASE("floating-point comparison with doctest::Approx") {
    SUBCASE("basic double comparison") {
        CHECK(doctest_sample::DoubleIt(2.5) == doctest::Approx(5.0));
        CHECK(doctest_sample::DoubleIt(0.0) == doctest::Approx(0.0));
        CHECK(doctest_sample::DoubleIt(-1.5) == doctest::Approx(-3.0));
    }

    SUBCASE("Approx with custom epsilon") {
        // epsilon を指定して許容誤差を調整する
        CHECK(std::sqrt(2.0) == doctest::Approx(1.41421356).epsilon(1e-6));
    }
}

// ─── サンプル: コンテナ比較 ───────────────────────────────────────────────────

TEST_CASE("container comparison") {
    SUBCASE("equal vectors") {
        const auto result = doctest_sample::MakeRange(1, 5);
        const std::vector<int> expected = {1, 2, 3, 4, 5};
        CHECK(result == expected);
    }

    SUBCASE("empty vector") {
        const auto result = doctest_sample::MakeRange(0, 0);
        CHECK(result.empty());
    }

    SUBCASE("negative start") {
        const auto result = doctest_sample::MakeRange(-2, 3);
        const std::vector<int> expected = {-2, -1, 0};
        CHECK(result == expected);
    }
}

// ─── サンプル: クラスとメソッドのテスト ─────────────────────────────────────

TEST_CASE("class method test") {
    const doctest_sample::Modulo mod7(7);

    SUBCASE("basic remainder") { CHECK(mod7.Remainder(15) == 1); }

    SUBCASE("divisible") { CHECK(mod7.Remainder(14) == 0); }

    SUBCASE("value less than divisor") { CHECK(mod7.Remainder(6) == 6); }
}

// ─── サンプル: 例外テスト ─────────────────────────────────────────────────────

TEST_CASE("exception testing") {
    SUBCASE("throws on zero divisor") {
        // std::vector::at は範囲外アクセスで std::out_of_range を投げる
        const std::vector<int> v = {1, 2, 3};
        int dummy = 0;
        CHECK_THROWS_AS(dummy = v.at(10), std::out_of_range);
        (void)dummy;
    }

    SUBCASE("does not throw") {
        const std::vector<int> v = {1, 2, 3};
        int val = 0;
        CHECK_NOTHROW(val = v.at(0));
        (void)val;
    }
}
