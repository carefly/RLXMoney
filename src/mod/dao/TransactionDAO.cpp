#include "mod/dao/TransactionDAO.h"
#include "mod/exceptions/MoneyException.h"
#include <SQLiteCpp/Statement.h>
#include <chrono>

namespace rlx_money {

TransactionDAO::TransactionDAO(DatabaseManager& dbManager) : mDbManager(dbManager) {}

bool TransactionDAO::createTransaction(const TransactionRecord& record) {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "INSERT INTO transactions (xuid, currency_id, amount, balance, type, description, timestamp, "
                          "related_xuid, transfer_id) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, record.xuid);
        stmt.bind(2, record.currencyId);
        stmt.bind(3, record.amount);
        stmt.bind(4, record.balance);
        stmt.bind(5, transactionTypeToString(record.type));
        stmt.bind(6, record.description);
        stmt.bind(7, record.timestamp);
        if (record.relatedXuid.has_value()) {
            stmt.bind(8, record.relatedXuid.value());
        } else {
            stmt.bind(8);
        }
        if (record.transferId.has_value()) {
            stmt.bind(9, record.transferId.value());
        } else {
            stmt.bind(9);
        }

        stmt.exec();
        return true;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("创建交易记录失败: " + std::string(e.what()));
    }
}

std::vector<TransactionRecord>
TransactionDAO::getPlayerTransactions(const std::string& xuid, const std::string& currencyId, int page, int pageSize)
    const {
    try {
        auto& db = mDbManager.getConnection();

        std::string sql;
        if (currencyId.empty()) {
            sql = "SELECT id, xuid, currency_id, amount, balance, type, description, timestamp, related_xuid, "
                  "transfer_id "
                  "FROM transactions WHERE xuid = ? ORDER BY timestamp DESC LIMIT ? OFFSET ?";
        } else {
            sql = "SELECT id, xuid, currency_id, amount, balance, type, description, timestamp, related_xuid, "
                  "transfer_id "
                  "FROM transactions WHERE xuid = ? AND currency_id = ? ORDER BY timestamp DESC LIMIT ? OFFSET ?";
        }

        int offset = (page - 1) * pageSize;

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);
        if (!currencyId.empty()) {
            stmt.bind(2, currencyId);
            stmt.bind(3, pageSize);
            stmt.bind(4, offset);
        } else {
            stmt.bind(2, pageSize);
            stmt.bind(3, offset);
        }

        std::vector<TransactionRecord> result;
        while (stmt.executeStep()) {
            result.push_back(buildTransactionRecordFromStatement(stmt));
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取玩家交易记录失败: " + std::string(e.what()));
    }
}

int TransactionDAO::getPlayerTransactionCount(const std::string& xuid) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT COUNT(*) FROM transactions WHERE xuid = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);

        if (stmt.executeStep()) {
            return stmt.getColumn(0).getInt();
        }

        return 0;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取玩家交易记录总数失败: " + std::string(e.what()));
    }
}

std::vector<TransactionRecord>
TransactionDAO::getPlayerTransactionsByType(const std::string& xuid, TransactionType type, int page, int pageSize)
    const {
    try {
        auto& db = mDbManager.getConnection();

        std::string typeStr = transactionTypeToString(type);
        const char* sql =
            "SELECT id, xuid, currency_id, amount, balance, type, description, timestamp, related_xuid, transfer_id "
            "FROM transactions WHERE xuid = ? AND type = ? ORDER BY timestamp DESC LIMIT ? OFFSET ?";

        int offset = (page - 1) * pageSize;

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);
        stmt.bind(2, typeStr);
        stmt.bind(3, pageSize);
        stmt.bind(4, offset);

        std::vector<TransactionRecord> result;
        while (stmt.executeStep()) {
            result.push_back(buildTransactionRecordFromStatement(stmt));
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("按类型获取玩家交易记录失败: " + std::string(e.what()));
    }
}

std::vector<TransactionRecord> TransactionDAO::getPlayerTransactionsByTimeRange(
    const std::string& xuid,
    int64_t            startTime,
    int64_t            endTime,
    int                page,
    int                pageSize
) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql =
            "SELECT id, xuid, currency_id, amount, balance, type, description, timestamp, related_xuid, transfer_id "
            "FROM transactions WHERE xuid = ? AND timestamp >= ? AND timestamp <= ? ORDER BY timestamp "
            "DESC LIMIT ? OFFSET ?";

        int offset = (page - 1) * pageSize;

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);
        stmt.bind(2, startTime);
        stmt.bind(3, endTime);
        stmt.bind(4, pageSize);
        stmt.bind(5, offset);

        std::vector<TransactionRecord> result;
        while (stmt.executeStep()) {
            result.push_back(buildTransactionRecordFromStatement(stmt));
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("按时间范围获取玩家交易记录失败: " + std::string(e.what()));
    }
}

std::vector<TransactionRecord> TransactionDAO::getRecentTransactions(int limit) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql =
            "SELECT id, xuid, currency_id, amount, balance, type, description, timestamp, related_xuid, transfer_id "
            "FROM transactions ORDER BY timestamp DESC LIMIT ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, limit);

        std::vector<TransactionRecord> result;
        while (stmt.executeStep()) {
            result.push_back(buildTransactionRecordFromStatement(stmt));
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取最近交易记录失败: " + std::string(e.what()));
    }
}

int TransactionDAO::getTotalTransactionCount() const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT COUNT(*) FROM transactions";

        SQLite::Statement stmt(db, sql);

        if (stmt.executeStep()) {
            return stmt.getColumn(0).getInt();
        }

        return 0;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取总交易次数失败: " + std::string(e.what()));
    }
}

int TransactionDAO::getTransactionCountByType(TransactionType type) const {
    try {
        auto& db = mDbManager.getConnection();

        std::string typeStr = transactionTypeToString(type);
        const char* sql     = "SELECT COUNT(*) FROM transactions WHERE type = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, typeStr);

        if (stmt.executeStep()) {
            return stmt.getColumn(0).getInt();
        }

        return 0;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("按类型获取交易次数失败: " + std::string(e.what()));
    }
}

int TransactionDAO::cleanupOldTransactions(int daysToKeep) {
    try {
        auto& db = mDbManager.getConnection();

        // 计算截止时间戳
        auto currentTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        int64_t cutoffTime = currentTime - (daysToKeep * 24 * 60 * 60);

        const char* sql = "DELETE FROM transactions WHERE timestamp < ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, cutoffTime);

        stmt.exec();
        return stmt.getChanges();

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("清理过期交易记录失败: " + std::string(e.what()));
    }
}

TransactionRecord TransactionDAO::buildTransactionRecordFromStatement(SQLite::Statement& stmt) const {
    TransactionRecord record;
    record.id          = stmt.getColumn(0).getInt();
    record.xuid        = stmt.getColumn(1).getString();
    record.currencyId  = stmt.getColumn(2).getString();
    record.amount      = stmt.getColumn(3).getInt();
    record.balance     = stmt.getColumn(4).getInt();
    record.type        = stringToTransactionType(stmt.getColumn(5).getString());
    record.description = stmt.getColumn(6).getString();
    record.timestamp   = stmt.getColumn(7).getInt64();
    if (!stmt.getColumn(8).isNull()) {
        record.relatedXuid = stmt.getColumn(8).getString();
    } else {
        record.relatedXuid = std::nullopt;
    }
    if (!stmt.getColumn(9).isNull()) {
        record.transferId = stmt.getColumn(9).getString();
    } else {
        record.transferId = std::nullopt;
    }
    return record;
}

std::optional<TransactionRecord>
TransactionDAO::executeQuerySingle(const std::string& sql, const std::vector<std::string>& params) const {
    try {
        auto&             db = mDbManager.getConnection();
        SQLite::Statement stmt(db, sql);

        for (size_t i = 0; i < params.size(); ++i) {
            stmt.bind(static_cast<int>(i + 1), params[i]);
        }

        if (stmt.executeStep()) {
            return buildTransactionRecordFromStatement(stmt);
        }

        return std::nullopt;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("执行查询失败: " + std::string(e.what()));
    }
}

std::vector<TransactionRecord>
TransactionDAO::executeQueryMultiple(const std::string& sql, const std::vector<std::string>& params) const {
    try {
        auto&             db = mDbManager.getConnection();
        SQLite::Statement stmt(db, sql);

        for (size_t i = 0; i < params.size(); ++i) {
            stmt.bind(static_cast<int>(i + 1), params[i]);
        }

        std::vector<TransactionRecord> result;
        while (stmt.executeStep()) {
            result.push_back(buildTransactionRecordFromStatement(stmt));
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("执行查询失败: " + std::string(e.what()));
    }
}

void TransactionDAO::bindParameters(SQLite::Statement& stmt, const std::vector<std::string>& params) const {
    for (size_t i = 0; i < params.size(); ++i) {
        stmt.bind(static_cast<int>(i + 1), params[i]);
    }
}

} // namespace rlx_money