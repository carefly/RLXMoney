#pragma once

#ifdef TESTING

#include <string>
#include <unordered_map>
#include <memory>

// Mock Player 类用于测试
class Player {
public:
    std::string xuid;
    std::string name;

    Player(const std::string& x, const std::string& n) : xuid(x), name(n) {
        mName = name; // 同步mName字段
    }

    // 获取XUID的方法
    const std::string& getXuid() const { return xuid; }

    // 模拟mName公共成员变量
    std::string mName;
};

namespace rlx_money {

/// @brief LeviLamina API 的Mock实现，用于测试环境
class LeviLaminaAPI {
private:
    static std::unordered_map<std::string, std::shared_ptr<Player>> xuidToPlayer;
    static std::unordered_map<std::string, std::shared_ptr<Player>> nameToPlayer;

public:
    /// @brief 添加Mock玩家数据
    static void addMockPlayer(const std::string& xuid, const std::string& name);

    /// @brief 清除所有Mock玩家数据
    static void clearMockPlayers();

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

#endif // TESTING