#pragma once

#include <string>

// 前向声明，避免包含LeviLamina头文件
class Player;

namespace rlx_money {

/// @brief LeviLamina API 封装类
class LeviLaminaAPI {
public:
    /// @brief 根据XUID获取玩家对象
    /// @param xuid 玩家XUID
    /// @return 玩家对象指针，如果不存在返回nullptr
    [[nodiscard]] static Player* getPlayerByXuid(const std::string& xuid);

    /// @brief 根据玩家名获取玩家对象
    /// @param name 玩家名
    /// @return 玩家对象指针，如果不存在返回nullptr
    [[nodiscard]] static Player* getPlayerByName(const std::string& name);

    /// @brief 根据XUID获取玩家名
    /// @param xuid 玩家XUID
    /// @return 玩家名，如果不存在返回空字符串
    [[nodiscard]] static std::string getPlayerNameByXuid(const std::string& xuid);

    /// @brief 根据玩家名获取XUID
    /// @param name 玩家名
    /// @return 玩家XUID，如果不存在返回空字符串
    [[nodiscard]] static std::string getXuidByPlayerName(const std::string& name);
};

} // namespace rlx_money