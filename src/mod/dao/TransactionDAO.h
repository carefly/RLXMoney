#pragma once

#include "mod/data/DataStructures.h"
#include "mod/database/DatabaseManager.h"
#include "mod/types/Types.h"
#include <SQLiteCpp/Statement.h>
#include <optional>
#include <vector>


namespace rlx_money {

/// @brief 交易记录数据访问对象类
class TransactionDAO {
public:
    /// @brief 构造函数
    /// @param dbManager 数据库管理器引用
    explicit TransactionDAO(DatabaseManager& dbManager);

    /// @brief 创建交易记录
    /// @param record 交易记录
    /// @return 是否创建成功
    bool createTransaction(const TransactionRecord& record);

    /// @brief 获取玩家交易历史
    /// @param xuid 玩家XUID
    /// @param currencyId 币种ID（可选，为空则查询所有币种）
    /// @param page 页码（从1开始）
    /// @param pageSize 每页大小
    /// @return 交易记录列表
    [[nodiscard]] std::vector<TransactionRecord>
    getPlayerTransactions(const std::string& xuid, const std::string& currencyId = "", int page = 1, int pageSize = 10)
        const;

    /// @brief 获取玩家交易记录总数
    /// @param xuid 玩家XUID
    /// @return 记录总数
    [[nodiscard]] int getPlayerTransactionCount(const std::string& xuid) const;

    /// @brief 根据交易类型获取玩家交易记录
    /// @param xuid 玩家XUID
    /// @param type 交易类型
    /// @param page 页码
    /// @param pageSize 每页大小
    /// @return 交易记录列表
    [[nodiscard]] std::vector<TransactionRecord>
    getPlayerTransactionsByType(const std::string& xuid, TransactionType type, int page = 1, int pageSize = 10) const;

    /// @brief 获取指定时间范围内的交易记录
    /// @param xuid 玩家XUID
    /// @param startTime 开始时间戳
    /// @param endTime 结束时间戳
    /// @param page 页码
    /// @param pageSize 每页大小
    /// @return 交易记录列表
    [[nodiscard]] std::vector<TransactionRecord> getPlayerTransactionsByTimeRange(
        const std::string& xuid,
        int64_t            startTime,
        int64_t            endTime,
        int                page     = 1,
        int                pageSize = 10
    ) const;

    /// @brief 获取最近的交易记录
    /// @param limit 限制数量
    /// @return 交易记录列表
    [[nodiscard]] std::vector<TransactionRecord> getRecentTransactions(int limit = 50) const;

    /// @brief 获取服务器总交易次数
    /// @return 总交易次数
    [[nodiscard]] int getTotalTransactionCount() const;

    /// @brief 获取指定交易类型的总次数
    /// @param type 交易类型
    /// @return 交易次数
    [[nodiscard]] int getTransactionCountByType(TransactionType type) const;

    /// @brief 清理过期的交易记录
    /// @param daysToKeep 保留天数
    /// @return 清理的记录数
    int cleanupOldTransactions(int daysToKeep = 90);

private:
    /// @brief 从查询结果构建交易记录
    /// @param stmt SQLite语句
    /// @return 交易记录
    TransactionRecord buildTransactionRecordFromStatement(SQLite::Statement& stmt) const;

    /// @brief 执行查询并返回单个结果
    /// @param sql SQL语句
    /// @param params 参数列表
    /// @return 交易记录
    [[nodiscard]] std::optional<TransactionRecord>
    executeQuerySingle(const std::string& sql, const std::vector<std::string>& params = {}) const;

    /// @brief 执行查询并返回多个结果
    /// @param sql SQL语句
    /// @param params 参数列表
    /// @return 交易记录列表
    [[nodiscard]] std::vector<TransactionRecord>
    executeQueryMultiple(const std::string& sql, const std::vector<std::string>& params = {}) const;

    /// @brief 绑定参数到预处理语句
    /// @param stmt 预处理语句
    /// @param params 参数列表
    void bindParameters(SQLite::Statement& stmt, const std::vector<std::string>& params) const;

    DatabaseManager& mDbManager;
};

} // namespace rlx_money