#pragma once

#include "ll/api/mod/NativeMod.h"


namespace rlx_money {

class RLXMoney {
public:
    static RLXMoney& getInstance();

    RLXMoney() : mSelf(*ll::mod::NativeMod::current()) {}

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    /// @return True if the mod is loaded successfully.
    [[nodiscard]] bool load();

    /// @return True if the mod is enabled successfully.
    [[nodiscard]] bool enable();

    /// @return True if the mod is disabled successfully.
    [[nodiscard]] bool disable();

    // TODO: Implement this method if you need to unload the mod.
    // /// @return True if the mod is unloaded successfully.
    // bool unload();

private:
    ll::mod::NativeMod& mSelf;
    bool                mInitialized = false;

    /// @brief 初始化所有组件
    /// @return 是否初始化成功
    [[nodiscard]] bool initializeComponents() const;

    /// @brief 清理所有组件
    void cleanupComponents() const;
};

} // namespace rlx_money