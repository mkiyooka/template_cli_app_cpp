#pragma once

#include <string_view>

#include <fmt/format.h>

namespace recording {

/**
 * @brief 解析データ記録の共通インターフェース
 *
 * モジュール別にデータ出力の有効・無効を制御し、
 * 出力先（ファイル・メモリ等）を DI で切り替えられる。
 *
 * フォーマットは呼び出し側で行い、文字列として渡す設計とする。
 * ただし Write() テンプレートヘルパーで fmt::format の利便性を維持する。
 *
 * @code
 * recorder.Write("{},{:.6f}", step, value);
 * @endcode
 */
class DataRecorder {
public:
    DataRecorder() = default;
    virtual ~DataRecorder() = default;

    DataRecorder(const DataRecorder &) = delete;
    DataRecorder &operator=(const DataRecorder &) = delete;
    DataRecorder(DataRecorder &&) = delete;
    DataRecorder &operator=(DataRecorder &&) = delete;

    /**
     * @brief 出力を有効化する
     */
    virtual void Enable() = 0;

    /**
     * @brief 出力を無効化する
     */
    virtual void Disable() = 0;

    /**
     * @brief 出力が有効かを返す
     */
    virtual bool IsEnabled() const = 0;

    /**
     * @brief 文字列をそのまま出力する
     * @param message 出力するメッセージ
     */
    virtual void Output(std::string_view message) = 0;

    /**
     * @brief 出力バッファをフラッシュする
     */
    virtual void Flush() = 0;

    /**
     * @brief fmt::format でフォーマットしてから Output() に渡す非仮想ヘルパー
     *
     * フォーマット文字列はコンパイル時にチェックされる。
     * IsEnabled() が false の場合はフォーマット処理自体をスキップする。
     *
     * @code
     * recorder.Write("{},{:.6f}", step, value);
     * @endcode
     */
    template <typename... Args>
    void Write(fmt::format_string<Args...> fmt_str, Args &&...args) {
        if (IsEnabled()) {
            Output(fmt::format(fmt_str, std::forward<Args>(args)...));
        }
    }
};

} // namespace recording
