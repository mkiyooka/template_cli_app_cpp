#include <template_cli_app_cpp/utility/yyjson_wrapper.hpp>

#include <iostream>
#include <string>
#include <vector>

int main() {
    // ── フラットオブジェクト ──────────────────────────────────────
    {
        utility::JsonBuilder builder;
        builder.Add("name", "Alice");
        builder.Add("age", 30);
        builder.Add("score", 98.6);
        builder.Add("active", true);

        std::cout << "[flat object]\n";
        std::cout << builder.Serialize(true) << "\n\n";
    }

    // ── 配列フィールド ────────────────────────────────────────────
    {
        utility::JsonBuilder builder;
        builder.Add("scores", std::vector<int>{95, 87, 92});
        builder.Add("tags", std::vector<std::string>{"new", "sale", "limited"});

        std::cout << "[array fields]\n";
        std::cout << builder.Serialize(true) << "\n\n";
    }

    // ── ネストオブジェクト ────────────────────────────────────────
    {
        utility::JsonBuilder builder;
        builder.Add("version", 2);

        auto inputs = builder.AddNested("inputs");
        builder.AddToNested(inputs, "x", 3.5);
        builder.AddToNested(inputs, "n", 5);

        auto results = builder.AddNested("results");
        builder.AddToNested(results, "doubled", 7.0);
        builder.AddToNested(results, "remainder", 1);

        builder.Add("sequence", std::vector<int>{1, 2, 3, 4, 5});

        std::cout << "[nested object]\n";
        std::cout << builder.Serialize(true) << "\n\n";
    }

    // ── Clear して再利用 ──────────────────────────────────────────
    {
        utility::JsonBuilder builder;
        builder.Add("step", 1);
        builder.Add("value", 0.5);
        std::cout << "[before clear] size=" << builder.Size() << "\n";

        builder.Clear();
        std::cout << "[after clear]  empty=" << (builder.Empty() ? "true" : "false") << "\n";

        builder.Add("step", 2);
        builder.Add("value", 1.0);
        std::cout << "[reused]\n";
        std::cout << builder.Serialize() << "\n\n";
    }

    // ── コンパクト vs プリティ ────────────────────────────────────
    {
        utility::JsonBuilder builder;
        builder.Add("key", "value");
        std::cout << "[compact] " << builder.Serialize(false) << "\n";
        std::cout << "[pretty]\n" << builder.Serialize(true) << "\n";
    }

    return 0;
}
