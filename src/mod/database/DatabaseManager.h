#pragma once

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <functional>
#include <memory>
#include <string>


namespace rlx_money {

/// @brief 数据库管理器类
class DatabaseManager {
public:
    /// @brief 获取单例实例
    /// @return 数据库管理器实例
    static DatabaseManager& getInstance();

    /// @brief 初始化数据库连接
    /// @param dbPath 数据库文件路径
    /// @return 是否初始化成功
    bool initialize(const std::string& dbPath);

    /// @brief 获取数据库连接
    /// @return 数据库连接对象
    [[nodiscard]] SQLite::Database& getConnection() const;

    /// @brief 执行事务
    /// @param transaction 事务函数
    /// @return 是否执行成功
    bool executeTransaction(const std::function<bool(SQLite::Database&)>& transaction);

    /// @brief 检查数据库是否已初始化
    /// @return 是否已初始化
    [[nodiscard]] bool isInitialized() const;

    /// @brief 关闭数据库连接
    void close();

    /// @brief 重置管理器状态（仅用于测试）
    /// @note 此方法仅用于测试，用于清理数据库连接状态
    void resetForTesting();

    /// @brief 获取数据库路径
    /// @return 数据库文件路径
    [[nodiscard]] const std::string& getDatabasePath() const;

    DatabaseManager(const DatabaseManager&)            = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

private:
    DatabaseManager() = default;
    ~DatabaseManager();


    /// @brief 创建数据库表结构
    /// @param db 数据库连接
    /// @return 是否创建成功
    bool createTables(SQLite::Database& db);

    /// @brief 创建玩家表
    /// @param db 数据库连接
    /// @return 是否创建成功
    bool createPlayersTable(SQLite::Database& db);

    /// @brief 创建玩家余额表
    /// @param db 数据库连接
    /// @return 是否创建成功
    bool createPlayerBalancesTable(SQLite::Database& db);

    /// @brief 创建交易记录表
    /// @param db 数据库连接
    /// @return 是否创建成功
    bool createTransactionsTable(SQLite::Database& db);

    /// @brief 创建索引
    /// @param db 数据库连接
    /// @return 是否创建成功
    bool createIndexes(SQLite::Database& db);

    /// @brief 配置数据库优化参数
    /// @param db 数据库连接
    /// @return 是否配置成功
    bool configureOptimization(SQLite::Database& db);

    std::unique_ptr<SQLite::Database> mDatabase;
    std::string                       mDatabasePath;
    bool                              mInitialized = false;
};

} // namespace rlx_money