#pragma once

#include "mod/dao/PlayerDAO.h"
#include "mod/dao/TransactionDAO.h"
#include <RLXMoney/data/DataStructures.h>
#include <RLXMoney/types/Types.h>
#include <optional>
#include <vector>


namespace rlx_money {

/// @brief 经济管理器类
class EconomyManager {
public:
    /// @brief 获取单例实例
    /// @return 经济管理器实例
    static EconomyManager& getInstance();

    /// @brief 初始化经济管理器
    /// @return 是否初始化成功
    bool initialize();

    /// @brief 获取玩家余额
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @return 玩家余额
    [[nodiscard]] std::optional<int> getBalance(const std::string& xuid, const std::string& currencyId) const;

    /// @brief 获取玩家所有币种余额
    /// @param xuid 玩家XUID
    /// @return 玩家余额列表
    [[nodiscard]] std::vector<PlayerBalance> getAllBalances(const std::string& xuid) const;

    /// @brief 设置玩家余额
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 新余额
    /// @param description 操作描述
    /// @return 是否操作成功
    bool
    setBalance(const std::string& xuid, const std::string& currencyId, int amount, const std::string& description = "");

    /// @brief 增加玩家金钱
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 增加金额
    /// @param description 操作描述
    /// @return 是否操作成功
    bool
    addMoney(const std::string& xuid, const std::string& currencyId, int amount, const std::string& description = "");

    /// @brief 扣除玩家金钱
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 扣除金额
    /// @param description 操作描述
    /// @return 是否操作成功
    bool reduceMoney(
        const std::string& xuid,
        const std::string& currencyId,
        int                amount,
        const std::string& description = ""
    );

    /// @brief 设置玩家余额（带操作者信息）
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 新余额
    /// @param operatorType 操作者类型
    /// @param operatorName 操作者名称
    /// @return 是否操作成功
    bool setBalance(
        const std::string& xuid,
        const std::string& currencyId,
        int                amount,
        OperatorType       operatorType,
        const std::string& operatorName = ""
    );

    /// @brief 增加玩家金钱（带操作者信息）
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 增加金额
    /// @param operatorType 操作者类型
    /// @param operatorName 操作者名称
    /// @return 是否操作成功
    bool addMoney(
        const std::string& xuid,
        const std::string& currencyId,
        int                amount,
        OperatorType       operatorType,
        const std::string& operatorName = ""
    );

    /// @brief 扣除玩家金钱（带操作者信息）
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 扣除金额
    /// @param operatorType 操作者类型
    /// @param operatorName 操作者名称
    /// @return 是否操作成功
    bool reduceMoney(
        const std::string& xuid,
        const std::string& currencyId,
        int                amount,
        OperatorType       operatorType,
        const std::string& operatorName = ""
    );

    /// @brief 玩家间转账（同币种）
    /// @param fromXuid 转出玩家XUID
    /// @param toXuid 转入玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 转账金额
    /// @param description 转账描述
    /// @return 是否转账成功
    bool transferMoney(
        const std::string& fromXuid,
        const std::string& toXuid,
        const std::string& currencyId,
        int                amount,
        const std::string& description = ""
    );

    /// @brief 初始化新玩家
    /// @param xuid 玩家XUID
    /// @param username 玩家用户名
    /// @return 是否初始化成功
    bool initializeNewPlayer(const std::string& xuid, const std::string& username);

    /// @brief 检查玩家是否存在
    /// @param xuid 玩家XUID
    /// @return 玩家是否存在
    [[nodiscard]] bool playerExists(const std::string& xuid) const;

    /// @brief 获取财富排行榜（按币种）
    /// @param currencyId 币种ID
    /// @param limit 返回数量限制
    /// @return 财富排行榜
    [[nodiscard]] std::vector<TopBalanceEntry> getTopBalanceList(const std::string& currencyId, int limit) const;

    /// @brief 获取玩家交易历史
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID（可选，为空则查询所有币种）
    /// @param page 页码
    /// @param pageSize 每页大小
    /// @return 交易记录列表
    [[nodiscard]] std::vector<TransactionRecord>
    getPlayerTransactions(const std::string& xuid, const std::string& currencyId = "", int page = 1, int pageSize = 10)
        const;

    /// @brief 获取玩家交易记录总数
    /// @param xuid 玩家XUID
    /// @return 记录总数
    [[nodiscard]] int getPlayerTransactionCount(const std::string& xuid) const;

    /// @brief 验证金额是否有效
    /// @param amount 金额
    /// @return 是否有效
    [[nodiscard]] bool isValidAmount(int amount) const;

    /// @brief 检查余额是否充足
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 需要的金额
    /// @return 余额是否充足
    [[nodiscard]] bool hasSufficientBalance(const std::string& xuid, const std::string& currencyId, int amount) const;

    /// @brief 获取服务器总财富（按币种）
    /// @param currencyId 币种ID
    /// @return 总财富
    [[nodiscard]] int getTotalWealth(const std::string& currencyId) const;

    /// @brief 获取玩家总数
    /// @return 玩家总数
    [[nodiscard]] int getPlayerCount() const;

    /// @brief 获取默认币种ID
    /// @return 默认币种ID
    [[nodiscard]] std::string getDefaultCurrencyId() const;

    /// @brief 同步配置文件中的币种到数据库（用于配置重载后）
    /// @return 是否同步成功
    bool syncCurrenciesFromConfig();

    /// @brief 重置管理器状态（仅用于测试）
    /// @note 此方法仅用于测试，用于清理单例状态以便测试之间隔离
    void resetForTesting();

    EconomyManager(const EconomyManager&)            = delete;
    EconomyManager& operator=(const EconomyManager&) = delete;

private:
    EconomyManager();
    ~EconomyManager() = default;


    /// @brief 创建交易记录
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 交易金额
    /// @param balance 交易后余额
    /// @param type 交易类型
    /// @param description 描述
    /// @param relatedXuid 关联玩家XUID
    /// @param transferId 转账ID
    /// @return 是否创建成功
    bool createTransactionRecord(
        const std::string&                xuid,
        const std::string&                currencyId,
        int                               amount,
        int                               balance,
        TransactionType                   type,
        const std::string&                description = "",
        const std::optional<std::string>& relatedXuid = std::nullopt,
        const std::optional<std::string>& transferId  = std::nullopt
    );

    /// @brief 获取当前时间戳
    /// @return 时间戳
    [[nodiscard]] int64_t getCurrentTimestamp() const;

    /// @brief 验证币种是否存在且启用
    /// @param currencyId 币种ID
    /// @return 是否有效
    [[nodiscard]] bool isValidCurrency(const std::string& currencyId) const;

    PlayerDAO      mPlayerDAO;
    TransactionDAO mTransactionDAO;
    bool           mInitialized = false;
};

} // namespace rlx_money