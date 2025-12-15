#pragma once

#include "ll/api/mod/NativeMod.h"

// 前向声明
namespace ll::event {
class PlayerJoinEvent;
class PlayerChatEvent;
}

namespace my_money_plugin {

class MyMoneyPlugin {
public:
    static MyMoneyPlugin& getInstance();

    MyMoneyPlugin() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @brief 加载插件
    [[nodiscard]] bool load();

    /// @brief 启用插件
    [[nodiscard]] bool enable();

    /// @brief 禁用插件
    [[nodiscard]] bool disable();

private:
    ll::mod::NativeMod& mSelf;

    /// @brief 处理玩家加入事件
    void onPlayerJoin(ll::event::PlayerJoinEvent& event);

    /// @brief 处理玩家聊天事件
    void onPlayerChat(ll::event::PlayerChatEvent& event);
};

} // namespace my_money_plugin