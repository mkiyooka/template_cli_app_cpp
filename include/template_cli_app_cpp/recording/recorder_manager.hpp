#pragma once

#include <memory>
#include <unordered_map>

#include "template_cli_app_cpp/recording/data_recorder.hpp"

namespace recording {

/**
 * @brief モジュール別レコーダーを管理するマネージャー
 *
 * キー型 Key（通常は enum class）でレコーダーを登録・参照する。
 * 未登録のキーにアクセスした場合は std::out_of_range を送出する。
 *
 * @tparam Key レコーダーを識別するキー型（enum class 等）
 *
 * @code
 * enum class Module { X, Y, Z };
 *
 * RecorderManager<Module> manager;
 * manager.RegisterRecorder(Module::X, std::make_shared<SpdlogRecorder>(loggerX));
 * manager[Module::X].Enable();
 * manager[Module::X].Write("{},{:.6f}", step, value);
 * @endcode
 */
template <typename Key>
class RecorderManager {
public:
    /**
     * @brief レコーダーを登録する
     * @param key      モジュールを識別するキー
     * @param recorder レコーダーの所有権（shared_ptr）
     */
    void RegisterRecorder(Key key, std::shared_ptr<DataRecorder> recorder) { recorders_[key] = std::move(recorder); }

    /**
     * @brief キーに対応するレコーダーへの参照を返す
     * @throws std::out_of_range キーが未登録の場合
     */
    DataRecorder &operator[](Key key) { return *recorders_.at(key); }

    /**
     * @brief キーに対応するレコーダーへの const 参照を返す
     * @throws std::out_of_range キーが未登録の場合
     */
    const DataRecorder &operator[](Key key) const { return *recorders_.at(key); }

    /**
     * @brief 全レコーダーのバッファをフラッシュする
     */
    void FlushAll() {
        for (auto &[key, rec] : recorders_) {
            rec->Flush();
        }
    }

private:
    struct KeyHash {
        std::size_t operator()(Key k) const noexcept { return std::hash<int>{}(static_cast<int>(k)); }
    };

    std::unordered_map<Key, std::shared_ptr<DataRecorder>, KeyHash> recorders_;
};

} // namespace recording
