#pragma once
#include <csv.hpp>
#include <tl/expected.hpp>
#include <functional>
#include <string>
#include <vector>

namespace utility {

/**
 * @brief フィルタ付き CSV 読み込みクラス
 *
 * csv-parser の CSVReader をラップし、列名ベースの直感的な API を提供する。
 * 列名からインデックスへの解決はファイルオープン直後に一度だけ行うため、
 * ループ内でハッシュ探索が発生しない（インデックスアクセス相当の性能）。
 *
 * 使用例:
 * @code
 * utility::CsvReader reader("data.csv");
 *
 * // return 版: 1 回限りの取得
 * auto result = reader.ReadFiltered(
 *     [](const csv::CSVRow& row) { return row["flag"].get<int>() == 1; },
 *     {"value_a", "value_b"});
 * if (result) { // 使用 }
 *
 * // out-param 版: ループ内で vector を再利用
 * std::vector<double> buf;
 * for (const auto& path : paths) {
 *     utility::CsvReader r(path);
 *     r.ReadFiltered(pred, {"value"}, buf); // buf は毎回 clear される
 *     process(buf);
 * }
 * @endcode
 */
class CsvReader {
public:
    /**
     * @brief コンストラクタ
     * @param path CSV ファイルパス
     */
    explicit CsvReader(std::string path) : path_(std::move(path)) {}

    /**
     * @brief フィルタ付き CSV 読み込み（double 出力・return 版）
     *
     * 述語が true を返す行の指定列を double として返す。
     * 列名からインデックスへの解決は内部で一度だけ行う。
     *
     * @note ループ外の 1 回限りの取得に適する。繰り返し呼び出す場合は
     *       out-param 版を使うと vector の再確保を避けられる。
     *
     * @param predicate   行を受け取り true を返す行のみ出力対象とする述語
     * @param output_cols 出力したい列名のリスト
     * @return 成功時: 出力対象行の指定列を列挙した double の配列
     *         （要素順: 行0の列0, 行0の列1, ..., 行1の列0, ...）
     *         失敗時: エラーメッセージ文字列
     */
    tl::expected<std::vector<double>, std::string> ReadFiltered(
        std::function<bool(const csv::CSVRow &)> predicate,
        const std::vector<std::string> &output_cols) const {
        csv::CSVReader csv_reader(path_);
        auto indices_result = ResolveIndices(csv_reader, output_cols);
        if (!indices_result) {
            return tl::unexpected(indices_result.error());
        }
        const auto &indices = *indices_result;
        std::vector<double> result;
        // cppcheck-suppress useStlAlgorithm - predicate フィルタを含むため std::transform に単純置換不可
        for (auto &row : csv_reader) {
            if (predicate(row)) {
                for (int idx : indices) {
                    result.push_back(row[idx].get<double>());
                }
            }
        }
        return result;
    }

    /**
     * @brief フィルタ付き CSV 読み込み（string 出力・return 版）
     *
     * `ReadFiltered` の string 版。型変換（文字列→double）が不要な場合に使用する。
     * 内部では `get<csv::string_view>()` でゼロコピー読み取りし、イテレータ進行前に
     * `std::string` にコピーして返す。
     *
     * @note ループ外の 1 回限りの取得に適する。繰り返し呼び出す場合は
     *       out-param 版を使うと vector の再確保を避けられる。
     *
     * @param predicate   行を受け取り true を返す行のみ出力対象とする述語
     * @param output_cols 出力したい列名のリスト
     * @return 成功時: 出力対象行の指定列を列挙した string の配列
     *         失敗時: エラーメッセージ文字列
     */
    tl::expected<std::vector<std::string>, std::string> ReadFilteredAsStrings(
        std::function<bool(const csv::CSVRow &)> predicate,
        const std::vector<std::string> &output_cols) const {
        csv::CSVReader csv_reader(path_);
        auto indices_result = ResolveIndices(csv_reader, output_cols);
        if (!indices_result) {
            return tl::unexpected(indices_result.error());
        }
        const auto &indices = *indices_result;
        std::vector<std::string> result;
        // cppcheck-suppress useStlAlgorithm - predicate フィルタを含むため std::transform に単純置換不可
        for (auto &row : csv_reader) {
            if (predicate(row)) {
                for (int idx : indices) {
                    // string_view はイテレータ進行後に無効化されるため string にコピー
                    result.emplace_back(row[idx].get<csv::string_view>());
                }
            }
        }
        return result;
    }

    /**
     * @brief フィルタ付き CSV 読み込み（double 出力・out-param 版）
     *
     * 外側でループ処理をする場合、@p out を毎回 clear + reserve して再利用できるため
     * return 版と比べて vector のヒープ再確保を抑制できる。
     *
     * @note 呼び出し前に @p out を clear する必要はない（関数内でクリアする）。
     *
     * @param predicate   行を受け取り true を返す行のみ出力対象とする述語
     * @param output_cols 出力したい列名のリスト
     * @param out         結果を書き込む vector（クリアしてから追記する）
     * @return 成功時: void、失敗時: エラーメッセージ文字列
     */
    tl::expected<void, std::string> ReadFiltered(
        std::function<bool(const csv::CSVRow &)> predicate,
        const std::vector<std::string> &output_cols,
        std::vector<double> &out) const {
        csv::CSVReader csv_reader(path_);
        auto indices_result = ResolveIndices(csv_reader, output_cols);
        if (!indices_result) {
            return tl::unexpected(indices_result.error());
        }
        const auto &indices = *indices_result;
        out.clear();
        // cppcheck-suppress useStlAlgorithm - predicate フィルタを含むため std::transform に単純置換不可
        for (auto &row : csv_reader) {
            if (predicate(row)) {
                for (int idx : indices) {
                    out.push_back(row[idx].get<double>());
                }
            }
        }
        return {};
    }

    /**
     * @brief フィルタ付き CSV 読み込み（string 出力・out-param 版）
     *
     * 外側でループ処理をする場合、@p out を毎回 clear + reserve して再利用できるため
     * return 版と比べて vector のヒープ再確保を抑制できる。
     *
     * @note 呼び出し前に @p out を clear する必要はない（関数内でクリアする）。
     *
     * @param predicate   行を受け取り true を返す行のみ出力対象とする述語
     * @param output_cols 出力したい列名のリスト
     * @param out         結果を書き込む vector（クリアしてから追記する）
     * @return 成功時: void、失敗時: エラーメッセージ文字列
     */
    tl::expected<void, std::string> ReadFilteredAsStrings(
        std::function<bool(const csv::CSVRow &)> predicate,
        const std::vector<std::string> &output_cols,
        std::vector<std::string> &out) const {
        csv::CSVReader csv_reader(path_);
        auto indices_result = ResolveIndices(csv_reader, output_cols);
        if (!indices_result) {
            return tl::unexpected(indices_result.error());
        }
        const auto &indices = *indices_result;
        out.clear();
        // cppcheck-suppress useStlAlgorithm - predicate フィルタを含むため std::transform に単純置換不可
        for (auto &row : csv_reader) {
            if (predicate(row)) {
                for (int idx : indices) {
                    // string_view はイテレータ進行後に無効化されるため string にコピー
                    out.emplace_back(row[idx].get<csv::string_view>());
                }
            }
        }
        return {};
    }

private:
    std::string path_;

    /**
     * @brief 列名リストをインデックスに解決する共通実装
     * @return 成功時: インデックスの vector、失敗時: エラーメッセージ文字列
     */
    static tl::expected<std::vector<int>, std::string> ResolveIndices(
        csv::CSVReader &csv_reader,
        const std::vector<std::string> &cols) {
        std::vector<int> indices;
        indices.reserve(cols.size());
        for (const auto &col : cols) {
            int idx = csv_reader.index_of(col);
            if (idx < 0) {
                return tl::unexpected<std::string>(
                    "utility::CsvReader: column not found: " + col);
            }
            indices.push_back(idx);
        }
        return indices;
    }
};

} // namespace utility
