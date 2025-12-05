#pragma once

#include "mod/data/DataStructures.h"
#include "mod/database/DatabaseManager.h"
#include <SQLiteCpp/Statement.h>
#include <optional>
#include <vector>


namespace rlx_money {

/// @brief 玩家数据访问对象类
class PlayerDAO {
public:
    /// @brief 构造函数
    /// @param dbManager 数据库管理器引用
    explicit PlayerDAO(DatabaseManager& dbManager);

    /// @brief 创建玩家记录
    /// @param playerData 玩家数据
    /// @return 是否创建成功
    bool createPlayer(const PlayerData& playerData);

    /// @brief 根据XUID获取玩家数据
    /// @param xuid 玩家XUID
    /// @return 玩家数据（可选）
    [[nodiscard]] std::optional<PlayerData> getPlayerByXuid(const std::string& xuid) const;

    /// @brief 根据用户名获取玩家数据
    /// @param username 玩家用户名
    /// @return 玩家数据（可选）
    [[nodiscard]] std::optional<PlayerData> getPlayerByUsername(const std::string& username) const;

    /// @brief 更新玩家用户名
    /// @param xuid 玩家XUID
    /// @param newUsername 新用户名
    /// @return 是否更新成功
    bool updateUsername(const std::string& xuid, const std::string& newUsername);

    /// @brief 获取玩家指定币种的余额
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @return 余额（可选）
    [[nodiscard]] std::optional<int> getBalance(const std::string& xuid, const std::string& currencyId) const;

    /// @brief 更新玩家指定币种的余额
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param newBalance 新余额
    /// @return 是否更新成功
    bool updateBalance(const std::string& xuid, const std::string& currencyId, int newBalance);

    /// @brief 获取玩家所有币种余额
    /// @param xuid 玩家XUID
    /// @return 玩家余额列表
    [[nodiscard]] std::vector<PlayerBalance> getAllBalances(const std::string& xuid) const;

    /// @brief 初始化玩家币种余额（如果不存在则创建）
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param initialBalance 初始余额
    /// @return 是否成功
    bool initializeBalance(const std::string& xuid, const std::string& currencyId, int initialBalance);

    /// @brief 获取财富排行榜（按币种）
    /// @param currencyId 币种ID
    /// @param limit 返回数量限制
    /// @return 玩家余额列表
    [[nodiscard]] std::vector<TopBalanceEntry> getTopBalanceList(const std::string& currencyId, int limit) const;

    /// @brief 检查玩家是否存在
    /// @param xuid 玩家XUID
    /// @return 是否存在
    [[nodiscard]] bool playerExists(const std::string& xuid) const;

    /// @brief 获取所有玩家数量
    /// @return 玩家总数
    [[nodiscard]] int getPlayerCount() const;

    /// @brief 获取指定币种的总财富
    /// @param currencyId 币种ID
    /// @return 服务器总财富
    [[nodiscard]] int getTotalWealth(const std::string& currencyId) const;

private:
    /// @brief 从查询结果构建玩家数据
    /// @param stmt SQLite语句
    /// @return 玩家数据
    PlayerData buildPlayerDataFromStatement(SQLite::Statement& stmt) const;

    DatabaseManager& mDbManager;
};

} // namespace rlx_money