#pragma once

namespace rlx_money {

/// @brief 玩家事件监听器
class PlayerEventListener {
public:
    /// @brief 注册所有事件监听器
    static void registerListeners();

    /// @brief 取消注册所有事件监听器
    static void unregisterListeners();
};

} // namespace rlx_money
