#pragma once

#include "mod/data/DataStructures.h"
#include <optional>
#include <string>
#include <vector>


// 导出宏：
// - 在DLL工程内定义 RLXMONEY_EXPORTS，则导出
// - 在使用方不定义则导入
// - 静态库不需要导出/导入，RLXMONEY_API为空
#ifdef RLXMONEY_STATIC
#define RLXMONEY_API
#elif defined(RLXMONEY_EXPORTS)
#define RLXMONEY_API __declspec(dllexport)
#else
#define RLXMONEY_API __declspec(dllimport)
#endif


namespace rlx_money {

/// @brief RLXMoney 公共 API
class RLXMONEY_API RLXMoneyAPI {
public:
    /// @brief 获取玩家余额
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @return 玩家余额，如果玩家不存在返回 std::nullopt
    [[nodiscard]] static std::optional<int> getBalance(const std::string& xuid, const std::string& currencyId);

    /// @brief 获取玩家所有币种余额
    /// @param xuid 玩家XUID
    /// @return 玩家余额列表
    [[nodiscard]] static std::vector<PlayerBalance> getAllBalances(const std::string& xuid);

    /// @brief 设置玩家余额
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 新余额
    /// @param description 操作描述
    /// @return 是否操作成功
    static bool
    setBalance(const std::string& xuid, const std::string& currencyId, int amount, const std::string& description = "");

    /// @brief 增加玩家金钱
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 增加金额
    /// @param description 操作描述
    /// @return 是否操作成功
    static bool
    addMoney(const std::string& xuid, const std::string& currencyId, int amount, const std::string& description = "");

    /// @brief 扣除玩家金钱
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 扣除金额
    /// @param description 操作描述
    /// @return 是否操作成功
    static bool reduceMoney(
        const std::string& xuid,
        const std::string& currencyId,
        int                amount,
        const std::string& description = ""
    );

    /// @brief 检查玩家是否存在
    /// @param xuid 玩家XUID
    /// @return 玩家是否存在
    [[nodiscard]] static bool playerExists(const std::string& xuid);

    /// @brief 玩家间转账（同币种）
    /// @param fromXuid 转出玩家XUID
    /// @param toXuid 转入玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 转账金额
    /// @param description 转账描述
    /// @return 是否转账成功
    static bool transferMoney(
        const std::string& fromXuid,
        const std::string& toXuid,
        const std::string& currencyId,
        int                amount,
        const std::string& description = ""
    );

    /// @brief 检查余额是否充足
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID
    /// @param amount 需要的金额
    /// @return 余额是否充足
    [[nodiscard]] static bool hasSufficientBalance(const std::string& xuid, const std::string& currencyId, int amount);

    /// @brief 获取财富排行榜（按币种）
    /// @param currencyId 币种ID
    /// @param limit 返回数量限制
    /// @return 财富排行榜
    [[nodiscard]] static std::vector<TopBalanceEntry> getTopBalanceList(const std::string& currencyId, int limit);

    /// @brief 获取玩家交易历史
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID（可选，为空则查询所有币种）
    /// @param page 页码
    /// @param pageSize 每页大小
    /// @return 交易记录列表
    [[nodiscard]] static std::vector<TransactionRecord>
    getPlayerTransactions(const std::string& xuid, const std::string& currencyId = "", int page = 1, int pageSize = 10);

    /// @brief 获取玩家交易记录总数
    /// @param xuid 玩家XUID
    /// @return 记录总数
    [[nodiscard]] static int getPlayerTransactionCount(const std::string& xuid);

    /// @brief 获取服务器总财富（按币种）
    /// @param currencyId 币种ID
    /// @return 总财富
    [[nodiscard]] static int getTotalWealth(const std::string& currencyId);

    /// @brief 获取玩家总数
    /// @return 玩家总数
    [[nodiscard]] static int getPlayerCount();

    /// @brief 验证金额是否有效
    /// @param amount 金额
    /// @return 是否有效
    [[nodiscard]] static bool isValidAmount(int amount);

    /// @brief 获取所有启用的币种ID列表
    /// @return 币种ID列表
    [[nodiscard]] static std::vector<std::string> getEnabledCurrencyIds();

    /// @brief 获取默认币种ID
    /// @return 默认币种ID
    [[nodiscard]] static std::string getDefaultCurrencyId();

    /// @brief 初始化系统（加载配置、初始化数据库等）
    /// @param configPath 配置文件路径
    /// @return 是否初始化成功
    static bool initialize(const std::string& configPath);

    /// @brief 检查系统是否已初始化
    /// @return 是否已初始化
    [[nodiscard]] static bool isInitialized();

    // 禁止实例化
    RLXMoneyAPI()                              = delete;
    ~RLXMoneyAPI()                             = delete;
    RLXMoneyAPI(const RLXMoneyAPI&)            = delete;
    RLXMoneyAPI& operator=(const RLXMoneyAPI&) = delete;
};

} // namespace rlx_money