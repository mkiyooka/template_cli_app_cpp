#pragma once
#include <csv.hpp>
#include <functional>
#include <stdexcept>
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
 * // double 取得
 * auto values = reader.ReadFiltered(
 *     [](const csv::CSVRow& row) { return row["flag"].get<int>() == 1; },
 *     {"value_a", "value_b"});
 *
 * // 文字列取得（型変換コストなし）
 * auto labels = reader.ReadFilteredAsStrings(
 *     [](const csv::CSVRow& row) { return row["flag"].get<int>() == 1; },
 *     {"category"});
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
     * @brief フィルタ付き CSV 読み込み（double 出力）
     *
     * 述語が true を返す行の指定列を double として返す。
     * 列名からインデックスへの解決は内部で一度だけ行う。
     *
     * @param predicate  行を受け取り true を返す行のみ出力対象とする述語
     * @param output_cols 出力したい列名のリスト
     * @return 出力対象行の指定列を列挙した double の配列
     *         要素順: 行0の列0, 行0の列1, ..., 行1の列0, ...
     * @throws std::invalid_argument 存在しない列名が output_cols に含まれる場合
     */
    std::vector<double> ReadFiltered(
        std::function<bool(const csv::CSVRow &)> predicate,
        const std::vector<std::string> &output_cols) const {
        csv::CSVReader csv_reader(path_);

        const auto indices = ResolveIndices(csv_reader, output_cols);

        std::vector<double> result;
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
     * @brief フィルタ付き CSV 読み込み（string 出力）
     *
     * `ReadFiltered` の string 版。型変換（文字列→double）が不要な場合に使用する。
     * 内部では `get<csv::string_view>()` でゼロコピー読み取りし、イテレータ進行前に
     * `std::string` にコピーして返す。
     *
     * @param predicate  行を受け取り true を返す行のみ出力対象とする述語
     * @param output_cols 出力したい列名のリスト
     * @return 出力対象行の指定列を列挙した string の配列
     * @throws std::invalid_argument 存在しない列名が output_cols に含まれる場合
     */
    std::vector<std::string> ReadFilteredAsStrings(
        std::function<bool(const csv::CSVRow &)> predicate,
        const std::vector<std::string> &output_cols) const {
        csv::CSVReader csv_reader(path_);

        const auto indices = ResolveIndices(csv_reader, output_cols);

        std::vector<std::string> result;
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

private:
    std::string path_;

    // 列名リストをインデックスに解決する共通実装
    static std::vector<int> ResolveIndices(
        csv::CSVReader &csv_reader,
        const std::vector<std::string> &cols) {
        std::vector<int> indices;
        indices.reserve(cols.size());
        for (const auto &col : cols) {
            int idx = csv_reader.index_of(col);
            if (idx < 0) {
                throw std::invalid_argument("utility::CsvReader: column not found: " + col);
            }
            indices.push_back(idx);
        }
        return indices;
    }
};

} // namespace utility
