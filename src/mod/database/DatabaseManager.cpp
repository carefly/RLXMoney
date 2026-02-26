#include "mod/database/DatabaseManager.h"
#include "mod/exceptions/MoneyException.h"
#include <SQLiteCpp/Exception.h>
#include <filesystem>

namespace rlx_money {

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::~DatabaseManager() { close(); }

bool DatabaseManager::initialize(const std::string& dbPath) {
    try {
        // 如果已初始化，检查路径是否相同
        if (mInitialized) {
            if (mDatabasePath == dbPath) {
                // 已初始化且路径相同，直接返回成功
                return true;
            } else {
                // 已初始化但路径不同，这是错误情况（在生产环境中不应该发生）
                throw DatabaseException("数据库已初始化，无法切换到不同路径: " + dbPath);
            }
        }

        // 首次初始化
        mDatabasePath = dbPath;

        // 确保数据库文件所在目录存在
        std::filesystem::path dbFilePath(dbPath);
        std::filesystem::path dbDir = dbFilePath.parent_path();
        if (!dbDir.empty() && !std::filesystem::exists(dbDir)) {
            std::filesystem::create_directories(dbDir);
        }

        // 创建数据库连接
        mDatabase = std::make_unique<SQLite::Database>(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        // 配置优化参数
        if (!configureOptimization(*mDatabase)) {
            throw DatabaseException("数据库优化配置失败");
        }

        // 创建表结构
        if (!createTables(*mDatabase)) {
            throw DatabaseException("创建数据库表失败");
        }

        mInitialized = true;
        return true;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("初始化数据库失败: " + std::string(e.what()));
    }
}

SQLite::Database& DatabaseManager::getConnection() const {
    if (!mInitialized || !mDatabase) {
        throw DatabaseException("数据库未初始化");
    }

    return *mDatabase;
}

bool DatabaseManager::executeTransaction(const std::function<bool(SQLite::Database&)>& transaction) {
    if (!mInitialized || !mDatabase) {
        throw DatabaseException("数据库未初始化");
    }

    try {
        // 使用 IMMEDIATE 事务避免并发冲突（单线程下同样适用）
        mDatabase->exec("BEGIN IMMEDIATE TRANSACTION;");

        bool result = transaction(*mDatabase);

        if (result) {
            mDatabase->exec("COMMIT;");
            return true;
        } else {
            mDatabase->exec("ROLLBACK;");
            return false;
        }

    } catch (const SQLite::Exception& e) {
        try {
            mDatabase->exec("ROLLBACK;");
        } catch (...) {}
        throw DatabaseException("事务执行失败: " + std::string(e.what()));
    } catch (const std::exception& e) {
        try {
            mDatabase->exec("ROLLBACK;");
        } catch (...) {}
        throw DatabaseException("事务执行失败: " + std::string(e.what()));
    } catch (...) {
        try {
            mDatabase->exec("ROLLBACK;");
        } catch (...) {}
        throw;
    }
}


bool DatabaseManager::isInitialized() const { return mInitialized && mDatabase != nullptr; }

void DatabaseManager::close() {
    if (mDatabase) {
        mDatabase.reset();
        mInitialized = false;
    }
}

void DatabaseManager::resetForTesting() {
    // 关闭数据库连接并重置所有状态
    close();
    mDatabasePath.clear();
}

const std::string& DatabaseManager::getDatabasePath() const { return mDatabasePath; }

bool DatabaseManager::createTables(SQLite::Database& db) {
    try {
        // Currency 现在只存储在配置文件中，不再需要 currencies 和 currency_configs 表
        return createPlayersTable(db) && createPlayerBalancesTable(db) && createTransactionsTable(db)
            && createIndexes(db);
    } catch (const SQLite::Exception& e) {
        throw DatabaseException("创建数据库表失败: " + std::string(e.what()));
    }
}

bool DatabaseManager::createPlayersTable(SQLite::Database& db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS players (
            xuid TEXT PRIMARY KEY,
            username TEXT NOT NULL,
            first_join_time INTEGER NOT NULL,
            created_at INTEGER NOT NULL,
            updated_at INTEGER NOT NULL
        )
    )";

    try {
        db.exec(sql);
        return true;
    } catch (const SQLite::Exception& e) {
        throw DatabaseException("创建玩家表失败: " + std::string(e.what()));
    }
}

bool DatabaseManager::createPlayerBalancesTable(SQLite::Database& db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS player_balances (
            xuid TEXT NOT NULL,
            currency_id TEXT NOT NULL,
            balance INTEGER NOT NULL DEFAULT 0,
            updated_at INTEGER NOT NULL,
            PRIMARY KEY (xuid, currency_id),
            FOREIGN KEY (xuid) REFERENCES players(xuid) ON DELETE CASCADE
            -- currency_id 不再引用 currencies 表，由配置文件验证
        )
    )";

    try {
        db.exec(sql);
        return true;
    } catch (const SQLite::Exception& e) {
        throw DatabaseException("创建玩家余额表失败: " + std::string(e.what()));
    }
}

bool DatabaseManager::createTransactionsTable(SQLite::Database& db) {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS transactions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            xuid TEXT NOT NULL,
            currency_id TEXT NOT NULL,
            amount INTEGER NOT NULL,
            balance INTEGER NOT NULL,
            type TEXT NOT NULL,
            description TEXT,
            timestamp INTEGER NOT NULL,
            related_xuid TEXT,
            transfer_id TEXT,
            FOREIGN KEY (xuid) REFERENCES players(xuid),
            -- currency_id 不再引用 currencies 表，由配置文件验证
            FOREIGN KEY (related_xuid) REFERENCES players(xuid)
        )
    )";

    try {
        db.exec(sql);
        return true;
    } catch (const SQLite::Exception& e) {
        throw DatabaseException("创建交易记录表失败: " + std::string(e.what()));
    }
}

bool DatabaseManager::createIndexes(SQLite::Database& db) {
    const char* indexes[] = {
        "CREATE INDEX IF NOT EXISTS idx_players_username ON players(username)",
        "CREATE INDEX IF NOT EXISTS idx_player_balances_xuid ON player_balances(xuid)",
        "CREATE INDEX IF NOT EXISTS idx_player_balances_currency ON player_balances(currency_id)",
        "CREATE INDEX IF NOT EXISTS idx_player_balances_balance ON player_balances(balance)",
        "CREATE INDEX IF NOT EXISTS idx_transactions_xuid ON transactions(xuid)",
        "CREATE INDEX IF NOT EXISTS idx_transactions_currency ON transactions(currency_id)",
        "CREATE INDEX IF NOT EXISTS idx_transactions_timestamp ON transactions(timestamp)",
        "CREATE INDEX IF NOT EXISTS idx_transactions_type ON transactions(type)",
        "CREATE INDEX IF NOT EXISTS idx_transactions_related_xuid ON transactions(related_xuid)",
        "CREATE INDEX IF NOT EXISTS idx_transactions_transfer_id ON transactions(transfer_id)"
    };

    try {
        for (const char* sql : indexes) {
            db.exec(sql);
        }
        return true;
    } catch (const SQLite::Exception& e) {
        throw DatabaseException("创建索引失败: " + std::string(e.what()));
    }
}

bool DatabaseManager::configureOptimization(SQLite::Database& db) {
    const char* optimizations[] = {// 单线程 + Windows 测试环境更适合 DELETE 模式，避免 wal/shm 文件占用
                                   "PRAGMA journal_mode = DELETE",
                                   "PRAGMA synchronous = NORMAL",
                                   "PRAGMA cache_size = 10000",
                                   "PRAGMA temp_store = MEMORY",
                                   "PRAGMA mmap_size = 268435456",
                                   "PRAGMA optimize"
    };

    bool allSuccess = true;
    for (const char* sql : optimizations) {
        try {
            db.exec(sql);
        } catch (const SQLite::Exception&) {
            allSuccess = false;
        }
    }
    return allSuccess;
}


} // namespace rlx_money