#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "template_cli_app_cpp/utility/yyjson_wrapper.hpp"

// JSON文字列をパースして検証するヘルパー
static nlohmann::json Parse(const std::string &s) { return nlohmann::json::parse(s); }

// ──────────────────────────────────────────────────────────────
// フラットオブジェクト
// ──────────────────────────────────────────────────────────────

TEST_CASE("JsonBuilder: flat object") {
    SUBCASE("int field") {
        utility::JsonBuilder b;
        b.Add("age", 30);
        auto j = Parse(b.Serialize());
        CHECK(j["age"] == 30);
    }

    SUBCASE("double field") {
        utility::JsonBuilder b;
        b.Add("score", 98.6);
        auto j = Parse(b.Serialize());
        CHECK(j["score"].get<double>() == doctest::Approx(98.6));
    }

    SUBCASE("float field is stored as double") {
        utility::JsonBuilder b;
        b.Add("ratio", 1.5F);
        auto j = Parse(b.Serialize());
        CHECK(j["ratio"].get<double>() == doctest::Approx(1.5));
    }

    SUBCASE("bool field true") {
        utility::JsonBuilder b;
        b.Add("active", true);
        auto j = Parse(b.Serialize());
        CHECK(j["active"] == true);
    }

    SUBCASE("bool field false") {
        utility::JsonBuilder b;
        b.Add("active", false);
        auto j = Parse(b.Serialize());
        CHECK(j["active"] == false);
    }

    SUBCASE("std::string field") {
        utility::JsonBuilder b;
        const std::string name = "Alice";
        b.Add("name", name);
        auto j = Parse(b.Serialize());
        CHECK(j["name"] == "Alice");
    }

    SUBCASE("const char* field") {
        utility::JsonBuilder b;
        b.Add("city", "Tokyo");
        auto j = Parse(b.Serialize());
        CHECK(j["city"] == "Tokyo");
    }

    SUBCASE("multiple fields") {
        utility::JsonBuilder b;
        b.Add("name", "Bob");
        b.Add("age", 25);
        b.Add("active", true);
        auto j = Parse(b.Serialize());
        CHECK(j["name"] == "Bob");
        CHECK(j["age"] == 25);
        CHECK(j["active"] == true);
        CHECK(b.Size() == 3);
    }
}

// ──────────────────────────────────────────────────────────────
// 配列フィールド
// ──────────────────────────────────────────────────────────────

TEST_CASE("JsonBuilder: array fields") {
    SUBCASE("vector<int>") {
        utility::JsonBuilder b;
        b.Add("scores", std::vector<int>{10, 20, 30});
        auto j = Parse(b.Serialize());
        REQUIRE(j["scores"].is_array());
        CHECK(j["scores"].size() == 3);
        CHECK(j["scores"][0] == 10);
        CHECK(j["scores"][1] == 20);
        CHECK(j["scores"][2] == 30);
    }

    SUBCASE("vector<std::string>") {
        utility::JsonBuilder b;
        b.Add("tags", std::vector<std::string>{"alpha", "beta", "gamma"});
        auto j = Parse(b.Serialize());
        REQUIRE(j["tags"].is_array());
        CHECK(j["tags"].size() == 3);
        CHECK(j["tags"][0] == "alpha");
        CHECK(j["tags"][1] == "beta");
        CHECK(j["tags"][2] == "gamma");
    }

    SUBCASE("empty vector<int>") {
        utility::JsonBuilder b;
        b.Add("vals", std::vector<int>{});
        auto j = Parse(b.Serialize());
        REQUIRE(j["vals"].is_array());
        CHECK(j["vals"].empty());
    }
}

// ──────────────────────────────────────────────────────────────
// ネストオブジェクト
// ──────────────────────────────────────────────────────────────

TEST_CASE("JsonBuilder: nested object") {
    SUBCASE("single nesting with scalar fields") {
        utility::JsonBuilder b;
        auto user = b.AddNested("user");
        b.AddToNested(user, "name", "John");
        b.AddToNested(user, "age", 30);
        b.AddToNested(user, "active", true);
        auto j = Parse(b.Serialize());
        REQUIRE(j["user"].is_object());
        CHECK(j["user"]["name"] == "John");
        CHECK(j["user"]["age"] == 30);
        CHECK(j["user"]["active"] == true);
    }

    SUBCASE("nested double and float") {
        utility::JsonBuilder b;
        auto meta = b.AddNested("meta");
        b.AddToNested(meta, "x", 1.25);
        b.AddToNested(meta, "ratio", 0.5F);
        auto j = Parse(b.Serialize());
        CHECK(j["meta"]["x"].get<double>() == doctest::Approx(1.25));
        CHECK(j["meta"]["ratio"].get<double>() == doctest::Approx(0.5));
    }

    SUBCASE("multiple independent nested objects") {
        utility::JsonBuilder b;
        auto inputs = b.AddNested("inputs");
        b.AddToNested(inputs, "x", 3.5);
        b.AddToNested(inputs, "n", 5);
        auto results = b.AddNested("results");
        b.AddToNested(results, "doubled", 7.0);
        b.AddToNested(results, "remainder", 3);
        auto j = Parse(b.Serialize());
        CHECK(j["inputs"]["x"].get<double>() == doctest::Approx(3.5));
        CHECK(j["inputs"]["n"] == 5);
        CHECK(j["results"]["doubled"].get<double>() == doctest::Approx(7.0));
        CHECK(j["results"]["remainder"] == 3);
    }

    SUBCASE("nested object coexists with top-level fields") {
        utility::JsonBuilder b;
        b.Add("version", 1);
        auto meta = b.AddNested("meta");
        b.AddToNested(meta, "author", "test");
        b.Add("enabled", true);
        auto j = Parse(b.Serialize());
        CHECK(j["version"] == 1);
        CHECK(j["meta"]["author"] == "test");
        CHECK(j["enabled"] == true);
    }

    SUBCASE("nested vector<int> via AddToNested") {
        utility::JsonBuilder b;
        auto data = b.AddNested("data");
        b.AddToNested(data, "values", std::vector<int>{1, 2, 3});
        auto j = Parse(b.Serialize());
        REQUIRE(j["data"]["values"].is_array());
        CHECK(j["data"]["values"][0] == 1);
        CHECK(j["data"]["values"][1] == 2);
        CHECK(j["data"]["values"][2] == 3);
    }

    SUBCASE("nested vector<std::string> via AddToNested") {
        utility::JsonBuilder b;
        auto info = b.AddNested("info");
        b.AddToNested(info, "tags", std::vector<std::string>{"a", "b"});
        auto j = Parse(b.Serialize());
        REQUIRE(j["info"]["tags"].is_array());
        CHECK(j["info"]["tags"][0] == "a");
        CHECK(j["info"]["tags"][1] == "b");
    }
}

// ──────────────────────────────────────────────────────────────
// 複合シナリオ（cli_cli11.cpp の RunOutputSample と同等）
// ──────────────────────────────────────────────────────────────

TEST_CASE("JsonBuilder: complex scenario") {
    utility::JsonBuilder b;
    b.Add("version", 2);
    b.Add("name", "demo");
    auto inputs = b.AddNested("inputs");
    b.AddToNested(inputs, "input", 3.5);
    b.AddToNested(inputs, "start", 1);
    b.AddToNested(inputs, "count", 5);
    b.AddToNested(inputs, "divisor", 7);
    b.AddToNested(inputs, "target_value", 15);
    auto results = b.AddNested("results");
    b.AddToNested(results, "doubled", 7.0);
    b.AddToNested(results, "remainder", 1);
    b.Add("sequence", std::vector<int>{1, 2, 3, 4, 5});

    auto j = Parse(b.Serialize());
    CHECK(j["version"] == 2);
    CHECK(j["name"] == "demo");
    CHECK(j["inputs"]["input"].get<double>() == doctest::Approx(3.5));
    CHECK(j["inputs"]["start"] == 1);
    CHECK(j["inputs"]["count"] == 5);
    CHECK(j["inputs"]["divisor"] == 7);
    CHECK(j["inputs"]["target_value"] == 15);
    CHECK(j["results"]["doubled"].get<double>() == doctest::Approx(7.0));
    CHECK(j["results"]["remainder"] == 1);
    REQUIRE(j["sequence"].is_array());
    CHECK(j["sequence"].size() == 5);
    CHECK(j["sequence"][4] == 5);
}

// ──────────────────────────────────────────────────────────────
// ユーティリティ
// ──────────────────────────────────────────────────────────────

TEST_CASE("JsonBuilder: Size and Empty") {
    utility::JsonBuilder b;
    CHECK(b.Empty());
    CHECK(b.Size() == 0);
    b.Add("x", 1);
    CHECK_FALSE(b.Empty());
    CHECK(b.Size() == 1);
}

TEST_CASE("JsonBuilder: Clear") {
    utility::JsonBuilder b;
    b.Add("x", 42);
    CHECK(b.Size() == 1);
    b.Clear();
    CHECK(b.Empty());
    // Clear後も正常にフィールドを追加できる
    b.Add("y", 99);
    auto j = Parse(b.Serialize());
    CHECK(j.contains("y"));
    CHECK_FALSE(j.contains("x"));
}

TEST_CASE("JsonBuilder: Serialize pretty") {
    utility::JsonBuilder b;
    b.Add("key", "value");
    const std::string compact = b.Serialize(false);
    const std::string pretty = b.Serialize(true);
    // pretty は改行を含む
    CHECK(pretty.find('\n') != std::string::npos);
    // どちらも有効なJSON
    CHECK_NOTHROW(Parse(compact));
    CHECK_NOTHROW(Parse(pretty));
}

TEST_CASE("JsonBuilder: move semantics") {
    utility::JsonBuilder b1;
    b1.Add("val", 10);
    const utility::JsonBuilder b2 = std::move(b1);
    auto j = Parse(b2.Serialize());
    CHECK(j["val"] == 10);
    // ムーブ後の b1 は空ドキュメントと同等（デストラクタが安全に動く）
}
