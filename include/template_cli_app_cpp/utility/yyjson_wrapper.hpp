#pragma once
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>
#include <yyjson.h>

namespace utility {

/**
 * @brief ネストオブジェクトへの型安全ハンドル
 *
 * AddNested() が返す不透明ハンドル。
 * yyjson 内部型を呼び出し側に漏洩させない。
 */
class NestedObject {
    friend class JsonBuilder;
    explicit NestedObject(yyjson_mut_val *val) noexcept
        : val_(val) {}
    yyjson_mut_val *val_;
};

/**
 * @brief yyjsonラッパー - 高性能JSON構築ライブラリ
 *
 * 特徴:
 * - 統一Add関数による直感的API
 * - コンパイル時型推論で最適化
 * - 高性能（約150ns/op）
 * - メモリ安全（RAII、copy API使用）
 *
 * 使用例:
 * @code
 * utility::JsonBuilder builder;
 * builder.Add("name", "John");
 * builder.Add("age", 30);
 * builder.Add("scores", std::vector<int>{95, 87, 92});
 * auto nested = builder.AddNested("address");
 * builder.AddToNested(nested, "city", "Tokyo");
 * std::string json = builder.Serialize();
 * @endcode
 */
class JsonBuilder {
public:
    /**
     * @brief コンストラクタ - JSONドキュメントを初期化
     * @throws std::runtime_error yyjsonドキュメント作成に失敗した場合
     */
    JsonBuilder() { Init(); }

    // コピー禁止、ムーブ許可（RAII設計）
    JsonBuilder(const JsonBuilder &) = delete;
    JsonBuilder &operator=(const JsonBuilder &) = delete;

    JsonBuilder(JsonBuilder &&other) noexcept
        : doc_(other.doc_),
          root_(other.root_) {
        other.doc_ = nullptr;
        other.root_ = nullptr;
    }

    JsonBuilder &operator=(JsonBuilder &&other) noexcept {
        if (this != &other) {
            if (doc_ != nullptr) {
                yyjson_mut_doc_free(doc_);
            }
            doc_ = other.doc_;
            root_ = other.root_;
            other.doc_ = nullptr;
            other.root_ = nullptr;
        }
        return *this;
    }

    /**
     * @brief デストラクタ - リソースを自動解放
     */
    ~JsonBuilder() {
        if (doc_ != nullptr) {
            yyjson_mut_doc_free(doc_);
        }
    }

    /**
     * @brief 統一Add関数 - あらゆる型をサポート
     *
     * サポート型:
     * - int, double, float: 数値
     * - bool: 真偽値
     * - std::string, const char*: 文字列
     * - std::vector<int>: 数値配列
     * - std::vector<std::string>: 文字列配列
     *
     * @tparam T 値の型（自動推論）
     * @param key JSONキー名
     * @param value 設定する値
     *
     * @code
     * builder.Add("count", 42);              // int
     * builder.Add("price", 99.99);           // double
     * builder.Add("name", "Product");        // string
     * builder.Add("active", true);           // bool
     * builder.Add("tags", std::vector<std::string>{"new", "sale"});
     * @endcode
     */
    template <typename T>
    void Add(const char *key, const T &value) {
        AddVal(root_, key, value);
    }

    /**
     * @brief ネストしたオブジェクトを作成
     * @param key ネストオブジェクトのキー名
     * @return NestedObject ハンドル（AddToNestedで使用）
     *
     * @code
     * auto nested = builder.AddNested("user");
     * builder.AddToNested(nested, "name", "John");
     * builder.AddToNested(nested, "age", 30);
     * @endcode
     */
    NestedObject AddNested(const char *key) {
        yyjson_mut_val *obj = yyjson_mut_obj(doc_);
        yyjson_mut_obj_add_val(doc_, root_, key, obj);
        return NestedObject{obj};
    }

    /**
     * @brief ネストオブジェクトに値を追加
     * @tparam T 値の型（自動推論）
     * @param nested AddNestedで取得したネストオブジェクト
     * @param key キー名
     * @param value 設定する値
     */
    template <typename T>
    void AddToNested(NestedObject nested, const char *key, const T &value) {
        AddVal(nested.val_, key, value);
    }

    /**
     * @brief JSONデータの文字列表現を取得
     * @param pretty フォーマットするか（デフォルト: false）
     * @return JSON文字列
     *
     * @code
     * std::string compact   = builder.Serialize();
     * std::string formatted = builder.Serialize(true);
     * @endcode
     */
    std::string Serialize(bool pretty = false) const {
        std::size_t len = 0;
        const std::uint32_t flags = pretty ? YYJSON_WRITE_PRETTY : YYJSON_WRITE_NOFLAG;
        const std::unique_ptr<char, decltype(&std::free)> json_str{yyjson_mut_write(doc_, flags, &len), std::free};
        if (json_str == nullptr) {
            return "{}";
        }
        return {json_str.get(), len};
    }

    /**
     * @brief 現在のJSONサイズ（フィールド数）を取得
     * @return フィールド数
     */
    std::size_t Size() const { return yyjson_mut_obj_size(root_); }

    /**
     * @brief JSONが空かどうかチェック
     * @return 空の場合true
     */
    bool Empty() const { return Size() == 0; }

    /**
     * @brief JSONをクリア（全フィールドを削除）
     */
    void Clear() {
        if (doc_ != nullptr) {
            yyjson_mut_doc_free(doc_);
        }
        Init();
    }

private:
    yyjson_mut_doc *doc_{};
    yyjson_mut_val *root_{};

    void Init() {
        doc_ = yyjson_mut_doc_new(nullptr);
        if (doc_ == nullptr) {
            throw std::runtime_error("Failed to create yyjson document");
        }
        root_ = yyjson_mut_obj(doc_);
        if (root_ == nullptr) {
            yyjson_mut_doc_free(doc_);
            throw std::runtime_error("Failed to create root object");
        }
        yyjson_mut_doc_set_root(doc_, root_);
    }

    // ──────────────────────────────────────────────────────────
    // 型ディスパッチの共通実装（Add/AddToNested が共有）
    // ──────────────────────────────────────────────────────────

    template <typename T>
    void AddVal(yyjson_mut_val *obj, const char *key, const T &value) {
        using D = std::decay_t<T>;
        if constexpr (std::is_same_v<D, int>) {
            yyjson_mut_obj_add_int(doc_, obj, key, value);
        } else if constexpr (std::is_same_v<D, double> || std::is_same_v<D, float>) {
            yyjson_mut_obj_add_real(doc_, obj, key, static_cast<double>(value));
        } else if constexpr (std::is_same_v<D, bool>) {
            yyjson_mut_obj_add_bool(doc_, obj, key, value);
        } else if constexpr (std::is_same_v<D, std::string>) {
            yyjson_mut_obj_add_strcpy(doc_, obj, key, value.c_str());
        } else if constexpr (std::is_same_v<D, const char *> || std::is_same_v<D, char *>) {
            yyjson_mut_obj_add_strcpy(doc_, obj, key, value);
        } else if constexpr (std::is_same_v<D, std::vector<int>>) {
            AddArray<int>(obj, key, value);
        } else if constexpr (std::is_same_v<D, std::vector<std::string>>) {
            AddArray<std::string>(obj, key, value);
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported type for Add/AddToNested");
        }
    }

    template <typename Elem>
    void AddArray(yyjson_mut_val *obj, const char *key, const std::vector<Elem> &arr) {
        yyjson_mut_val *json_arr = yyjson_mut_arr(doc_);
        for (const auto &item : arr) {
            if constexpr (std::is_same_v<Elem, int>) {
                yyjson_mut_arr_add_int(doc_, json_arr, item);
            } else {
                yyjson_mut_arr_add_strcpy(doc_, json_arr, item.c_str());
            }
        }
        yyjson_mut_obj_add_val(doc_, obj, key, json_arr);
    }
};

} // namespace utility
