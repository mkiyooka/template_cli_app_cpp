# ビルドシステム

このドキュメントでは、プロジェクトのビルドシステム・開発ツール・ディレクトリ構成を説明する。

## 概要

| 項目                 | 内容              |
| -------------------- | ----------------- |
| ビルドシステム       | CMake 4.1 + Ninja |
| 環境管理             | pixi              |
| C++ 標準             | C++17             |
| コンパイラキャッシュ | ccache            |

## pixi タスク一覧

日常的な開発作業はすべて `pixi run <タスク名>` で実行できる。

pixi の環境は 2 種類ある。

| 環境      | セットアップ              | 用途                                           |
| --------- | ------------------------- | ---------------------------------------------- |
| `default` | `pixi install`            | ビルド・テストのみ                             |
| `dev`     | `pixi install -e dev`     | 静的解析・カバレッジ・スペルチェック等を含む   |

品質ツール・カバレッジ・ASan タスクは `dev` 環境でのみ利用できる（`pixi run -e dev <タスク名>`）。

### ビルド関連

| タスク         | 説明                                                          |
| -------------- | ------------------------------------------------------------- |
| `config`       | リリースビルドの CMake 設定（`build/`）                       |
| `config-debug` | デバッグビルドの CMake 設定（`build/`）                       |
| `build`        | ビルド                                                        |
| `test`         | テスト実行                                                    |
| `clean`        | 全ビルドディレクトリ（`build/`・`build-asan/`・`build-coverage/`）のクリーン |

### コード品質（dev 環境: `pixi run -e dev <タスク名>`）

| タスク         | 説明                                       |
| -------------- | ------------------------------------------ |
| `format`       | clang-format によるコード整形              |
| `lint`         | clang-tidy による静的解析                  |
| `run-cppcheck` | cppcheck による静的解析                    |
| `fullcheck`    | typos + lint + run-cppcheck をまとめて実行 |

### カバレッジ（dev 環境: `pixi run -e dev coverage`）

| タスク     | 説明                                                           |
| ---------- | -------------------------------------------------------------- |
| `coverage` | 設定 → テスト実行 → プロファイルデータ統合 → レポート生成まで一括実行 |

### サニタイザ（Linux のみ、dev 環境: `pixi run -e dev asan`）

| タスク   | 説明                                                             |
| -------- | ---------------------------------------------------------------- |
| `asan`   | ASan + UBSan ビルド → テスト実行（`build-asan/`、GCC を使用）   |

### valgrind（Linux のみ、要システムインストール）

| タスク     | 説明                                       |
| ---------- | ------------------------------------------ |
| `valgrind` | 全テストバイナリを valgrind（memcheck）で実行 |

> **注意**: pixi 環境の valgrind はシステムの動的リンカ（`/lib64/ld-linux-x86-64.so.2`）と互換性がないため、システムパッケージで別途インストールする必要がある。
>
> ```bash
> sudo apt-get install valgrind   # Ubuntu / Debian
> ```

## CMake プリセット

`CMakePresets.json` で以下のプリセットを定義している。

| プリセット       | ビルドタイプ   | ビルドディレクトリ | 説明                       |
| ---------------- | -------------- | ------------------ | -------------------------- |
| `release`        | Release        | `build/`           | 最適化ビルド               |
| `debug`          | Debug          | `build/`           | デバッグシンボル付きビルド |
| `relwithdebinfo` | RelWithDebInfo | `build/`           | 最適化 + デバッグシンボル  |
| `coverage`       | Debug          | `build-coverage/`  | カバレッジ計測用ビルド     |

```bash
# プリセットを直接使う場合
cmake --preset=release
cmake --build build --preset=release
ctest --preset=release
```

## CMake モジュール構成

`cmake/` 以下のモジュールを目的別に分けて管理している。

- `cmake/`
    - `local-or-fetch.cmake` — `add_external_package()` ヘルパー。`third_party/<名前>` があればローカルを使い、なければ FetchContent でダウンロードする
    - `dependencies-app.cmake` — アプリケーション用サードパーティライブラリの定義
    - `dependencies-test.cmake` — テスト・ベンチマーク用ライブラリの定義
    - `coverage-flags.cmake` — カバレッジ計測用コンパイルフラグ（`add_subdirectory` より前に include する必要がある）
    - `coverage.cmake` — カバレッジ計測・レポート生成ターゲットの定義
    - `quality-setup.cmake` — コード品質ツールのセットアップエントリポイント
    - `quality-tools.cmake` — clang-format / clang-tidy / cppcheck ターゲットの定義
    - `custom-targets.cmake` — `run-tests`・`show-help` などのカスタムターゲット
    - `collect-fetchcontent-licenses.cmake` — サードパーティライブラリのライセンスファイル収集

## ビルド成果物

ビルド後の主要な成果物は以下の場所に生成される。

- `build/template_cli_app_cpp` — メインの実行ファイル
- `build/test_*` — テストバイナリ
- `build/bench_*` — ベンチマークバイナリ
- `build/compile_commands.json` — LSP / clangd 向けの補完データベース（エディタはここを直接参照する）

## コンパイラ設定

### リンカの自動選択

CMakeLists.txt でリンカを自動選択する。GCC / Clang 系コンパイラの場合、以下の順で試みる。

1. mold
2. lld
3. システムデフォルト

`LINKER` キャッシュ変数で明示的に指定することもできる。

```bash
cmake --preset=release -DLINKER=lld
```

AppleClang は `-fuse-ld=` オプション非対応のためリンカ切り替えは行われない。

### ccache

`ccache` が PATH 上にある場合、自動的にコンパイラランチャーとして設定される。
再ビルド時間を大幅に短縮できる。

## カバレッジ

LLVM ソースベースカバレッジ（`-fprofile-instr-generate -fcoverage-mapping`）を使用する。

### 仕組み

1. `cmake/coverage-flags.cmake` でコンパイルフラグを設定（`add_subdirectory` より前）
2. テストバイナリを `LLVM_PROFILE_FILE` 環境変数付きで実行し `.profraw` を生成
3. `llvm-profdata merge` で `.profraw` を統合して `.profdata` を生成
4. `llvm-cov report` でターミナルにテキストレポートを出力
5. `llvm-cov show --format=html` で `build-coverage/coverage-html/` に HTML レポートを生成

### 実行手順

```bash
pixi run -e dev coverage   # 設定 → テスト実行 → レポート生成
```

### レポート対象の選択

`llvm-cov` の `--ignore-filename-regex` オプションで以下のパスをレポートから除外している。

| 除外パターン | 理由 |
| ------------ | ---- |
| `.*/build-coverage/.*` | ビルドディレクトリ内の生成ファイル・サードパーティヘッダ |
| `.*/third_party/.*` | ローカルキャッシュのサードパーティライブラリ |
| `.*/.pixi/.*` | pixi 環境の標準ライブラリ・ランタイムヘッダ |
| `.*/tests/.*` | テストコード自体（計測対象ではない） |

これにより `src/` と `include/` のプロダクションコードのみがレポートに表示される。

フィルタは `cmake/coverage.cmake` の `--ignore-filename-regex` で管理している。

### 注意事項

- カバレッジビルドは `build-coverage/` を使用する（`build/` とは別）
- macOS では AppleClang、Linux では pixi の clang（`CC=clang CXX=clang++`）を使用する
- `llvm-tools` パッケージ（`llvm-cov`・`llvm-profdata`）が pixi の `dev` 環境に含まれている

## コード品質ツール

### clang-format

`.clang-format` の設定に従いコードを整形する。

```bash
pixi run -e dev format      # 整形を適用
cmake --build build --target format-dry  # 変更なしで確認のみ
```

### clang-tidy

`.clang-tidy` の設定に従い静的解析を行う。
`run-clang-tidy` が利用可能な場合は並列実行される（コア数の半分）。

```bash
pixi run -e dev lint
```

### cppcheck

ソースコードとヘッダファイルに対して静的解析を実行する。

```bash
pixi run -e dev run-cppcheck          # 通常実行
cmake --build build --target run-cppcheck-verbose  # 詳細出力
```

### typos

ソースコード中のスペルミスを検出する。

```bash
pixi run -e dev typos
```

## サードパーティライブラリの管理

`add_external_package()` ヘルパー（`cmake/local-or-fetch.cmake`）を使って依存関係を管理する。

- `third_party/<ライブラリ名>-<バージョン>/` ディレクトリが存在する場合: ローカルのソースを使用
- ディレクトリが存在しない場合: FetchContent で自動ダウンロード

新しいライブラリを追加する場合は `cmake/dependencies-app.cmake`（または `dependencies-test.cmake`）に以下の形式で追記する。

```cmake
add_external_package(mylib third_party/mylib-1.0.0
    URL https://github.com/example/mylib/archive/refs/tags/v1.0.0.tar.gz
    URL_HASH SHA256=...
)
FetchContent_MakeAvailable(mylib)
```

ライセンスファイルは以下のコマンドで収集できる。

```bash
cmake --build build --target collect-licenses
# → build/third_party_licenses/ に収集される
```

## 対応プラットフォーム詳細

### macOS

- コンパイラ: AppleClang（システムデフォルト）
- リンカ: system
- pixi の `clang++` は macOS SDK のリンカと非互換のため使用しない
- カバレッジは `config-coverage` タスクで自動的に AppleClang を使用

### Linux x86-64

- コンパイラ: GCC 15（デフォルト）または Clang 21
- リンカ: mold（高速リンク）
- カバレッジビルドは `[target.linux-64.tasks.config-coverage]` で `CC=clang CXX=clang++` を明示指定
