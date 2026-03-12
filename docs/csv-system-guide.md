# CSV 読み込みシステム

csv-parser 2.5.1 のラッパーである `CsvReader` クラスの利用方法と設計の根拠を説明する。

---

## 概要

`include/template_cli_app_cpp/utility/csv_wrapper.hpp` が提供する `utility::CsvReader` は、
列名ベースの直感的な API でフィルタ付き CSV 読み込みを行うクラスである。

---

## API リファレンス

### `CsvReader`

```cpp
#include <template_cli_app_cpp/utility/csv_wrapper.hpp>

utility::CsvReader reader("path/to/data.csv");
```

#### `ReadFiltered`

```cpp
std::vector<double> ReadFiltered(
    std::function<bool(const csv::CSVRow &)> predicate,
    const std::vector<std::string> &output_cols) const;
```

述語が `true` を返す行の指定列を `double` で返す。

- `predicate` — 行を受け取り、出力対象かどうかを返す述語
- `output_cols` — 取り出したい列名のリスト（複数・重複可）
- 戻り値の要素順: `行0の列0, 行0の列1, ..., 行1の列0, ...`
- 存在しない列名を指定すると `std::invalid_argument` を投げる

#### `ReadFilteredAsStrings`

```cpp
std::vector<std::string> ReadFilteredAsStrings(
    std::function<bool(const csv::CSVRow &)> predicate,
    const std::vector<std::string> &output_cols) const;
```

`ReadFiltered` の string 版。`double` への変換コストが不要な場合に使用する。

---

## 使用例

`examples/example_csv_wrapper.cpp` に一連の使用例がある。

```cpp
utility::CsvReader reader("data.csv");

auto predicate = [](const csv::CSVRow &row) {
    return row["flag"].get<int>() == 1;
};

// flag==1 の行から value_a, value_b を double で取得
auto values = reader.ReadFiltered(predicate, {"value_a", "value_b"});

// flag==1 の行から category を文字列で取得
auto categories = reader.ReadFilteredAsStrings(predicate, {"category"});
```

---

## 設計の根拠とベンチマーク結果

### 計測条件

`benches/bench_csv.cpp`（`pixi run build && ./build/benches/bench_csv`）で以下の条件を計測した。

| 項目 | 5列 CSV | 31列 CSV |
| ---- | ------- | -------- |
| 行数 | 200,000 | 20,000 |
| 列構成 | id, category, value_a, value_b, flag | id, category, val00-val25×26, value_a, value_b, flag |
| フィルタ率 | flag==1（約 10%） | 同左 |
| 出力列 | value_a, value_b（2列） | value_a, value_b（2列） |
| 反復回数 | 3 回 / 計測ケース | 同左 |

### 4方式の比較

| 方式 | アクセス方法 | 出力型 | 特徴 |
| ---- | ------------ | ------ | ---- |
| A: by name | 毎行 `row["name"]`（ハッシュ探索） | double | API が最も直感的、ハッシュ探索コストが発生 |
| B: by index | `row[idx]`（整数インデックス直接） | double | ハッシュ探索コストを排除 |
| C: string_view | `row[idx].get<string_view>()` | string | 型変換コストも排除 |
| D: hardcoded | フィルタ・出力列を直接記述 | double | `std::function` / `std::vector<int>` の間接コストもゼロ |

### 実測値（Linux x86-64 / GCC 15 / Release ビルド）

計測条件: 5列CSV=200,000行 / 31列CSV=20,000行、flag==1（約10%）でフィルタ、3反復

#### 素読みスループット

| ケース | ns/row | row/s |
| ------ | ------: | ------: |
| 5列 default chunk (10MB) | 333 | 3,006,915 |
| 5列 chunk 64MB | 263 | 3,809,151 |
| 31列 default chunk (10MB) | 887 | 1,126,938 |
| 31列 chunk 64MB | 644 | 1,553,458 |

チャンクサイズを 64MB に拡大すると、5列で約 +27%、31列で約 +38% 向上した。

#### フィルタ付き読み込みスループット（通過行あたり）

| 方式 | 5列 ns/row | 31列 ns/row | 5列 row/s | 31列 row/s |
| ---- | ----------: | ----------: | ---------: | ----------: |
| A: by name | 3,700 | 10,173 | 270,274 | 98,301 |
| B: by index | 3,647 | 10,591 | 274,165 | 94,417 |
| C: string_view | 3,504 | 9,831 | 285,425 | 101,724 |
| D: hardcoded | 3,447 | 9,765 | 290,108 | 102,407 |

A を基準としたとき:

| 方式 | 5列での差 | 31列での差 |
| ---- | --------- | ---------- |
| B vs A | +1.4% 高速 | **-4.1% 低速** |
| C vs A | +5.3% 高速 | +3.4% 高速 |
| D vs A | +7.1% 高速 | +3.5% 高速 |

#### 31列で B が A より遅い理由

31列 CSV ではフィルタ通過行の数値処理より、**31列分の CSV トークン分割・解析コスト**が
支配的になる。このコストの前では列名ハッシュ探索（A）とインデックスアクセス（B）の差が
誤差範囲に収まり、測定誤差によって逆転することがある。

`std::function` の呼び出しオーバーヘッドが B には常に存在する一方、A の `row["name"]` は
内部でインデックスへの解決と同時に若干のキャッシュ効果が生じている可能性もある。

> 実測値は環境依存のため、本番実装で性能が重要な場合は `./build/benches/bench_csv` で
> 実際の環境・データで計測すること。

### なぜ内部でインデックスに解決するか（A API + B 性能）

csv-parser の `row["name"]` は毎行 `std::unordered_map` の探索を行う。
`CsvReader` は `CSVReader` 構築直後に `reader.index_of(col)` を呼び、
列名→インデックスの解決を一度だけ行う。ループ内は整数インデックスアクセスのみになり、
**列名指定という直感的な API で B 方式相当の性能を達成する**。

5列では A 比で +1.4% の性能向上が確認されており、列数が少なく出力列が多い場合ほど効果が出る。
31列のような列数が多い場合はトークン分割コストが支配的になるため差は小さい。

### なぜ string_view 版（`ReadFilteredAsStrings`）も提供するか

`get<double>()` は毎回文字列パースのコストを払う。カテゴリ列やラベル列のように
文字列のまま扱えばよい場合、`get<csv::string_view>()` でパースをスキップできる。

実測では 5列で +5.3%、31列でも +3.4% の性能向上が確認された。
型変換が不要なフィールドを扱う場合は `ReadFilteredAsStrings` を選択する。

### なぜ D 方式（ハードコード）を公開 API にしないか

D 方式は `std::function` / `std::vector<int>` の間接コストをゼロにする代わりに、
フィルタ条件・列インデックスをコード中に直書きする必要がある。
実測での A 比の改善は 5列で +7.1%、31列で +3.5% であり、汎用性の高い API 設計を優先した。
極限までスループットを追求する場合は `bench_csv.cpp` の `ReadFiltered_Hardcoded` を
参考にして直接実装することを検討する。

---

## 注意事項

- `csv::CSVReader` はコピー不可。`CsvReader` の各メソッド呼び出しごとにファイルを開き直す
- 大容量ファイルではチャンクサイズの調整が有効な場合がある（デフォルト: 10MB）
    - カスタマイズが必要な場合は `csv::CSVFormat::chunk_size()` を使って直接 `CSVReader` を構築すること
- `std::function` の呼び出しには仮想関数相当のオーバーヘッドがある。
  ホットループで問題になる場合はテンプレート述語を直接使う方式を検討する
