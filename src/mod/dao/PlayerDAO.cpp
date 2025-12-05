#include "mod/dao/PlayerDAO.h"
#include "mod/exceptions/MoneyException.h"
#include <SQLiteCpp/Statement.h>
#include <chrono>

namespace rlx_money {

PlayerDAO::PlayerDAO(DatabaseManager& dbManager) : mDbManager(dbManager) {}

bool PlayerDAO::createPlayer(const PlayerData& playerData) {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "INSERT INTO players (xuid, username, first_join_time, created_at, updated_at) "
                          "VALUES (?, ?, ?, ?, ?)";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, playerData.xuid);
        stmt.bind(2, playerData.username);
        stmt.bind(3, playerData.firstJoinTime);
        stmt.bind(4, playerData.createdAt);
        stmt.bind(5, playerData.updatedAt);

        stmt.exec();
        return true;

    } catch (const SQLite::Exception& e) {
        // SQLite 约束违反错误码通常是 19 (SQLITE_CONSTRAINT)
        if (e.getErrorCode() == 19) {
            throw DatabaseException("玩家已存在: " + playerData.xuid);
        }
        throw DatabaseException("创建玩家记录失败: " + std::string(e.what()));
    }
}

std::optional<PlayerData> PlayerDAO::getPlayerByXuid(const std::string& xuid) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT xuid, username, first_join_time, created_at, updated_at FROM players WHERE xuid = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);

        if (stmt.executeStep()) {
            return buildPlayerDataFromStatement(stmt);
        }

        return std::nullopt;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取玩家数据失败: " + std::string(e.what()));
    }
}

std::optional<PlayerData> PlayerDAO::getPlayerByUsername(const std::string& username) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql =
            "SELECT xuid, username, first_join_time, created_at, updated_at FROM players WHERE username = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, username);

        if (stmt.executeStep()) {
            return buildPlayerDataFromStatement(stmt);
        }

        return std::nullopt;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("根据用户名获取玩家数据失败: " + std::string(e.what()));
    }
}

std::optional<int> PlayerDAO::getBalance(const std::string& xuid, const std::string& currencyId) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT balance FROM player_balances WHERE xuid = ? AND currency_id = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);
        stmt.bind(2, currencyId);

        if (stmt.executeStep()) {
            return stmt.getColumn(0).getInt();
        }

        return std::nullopt;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取玩家余额失败: " + std::string(e.what()));
    }
}

bool PlayerDAO::updateBalance(const std::string& xuid, const std::string& currencyId, int newBalance) {
    try {
        auto& db = mDbManager.getConnection();

        auto currentTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        // 使用 INSERT OR REPLACE 来更新或创建余额记录
        const char* sql = "INSERT OR REPLACE INTO player_balances (xuid, currency_id, balance, updated_at) "
                          "VALUES (?, ?, ?, ?)";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);
        stmt.bind(2, currencyId);
        stmt.bind(3, newBalance);
        stmt.bind(4, currentTime);

        stmt.exec();
        return true;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("更新玩家余额失败: " + std::string(e.what()));
    }
}

std::vector<PlayerBalance> PlayerDAO::getAllBalances(const std::string& xuid) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT xuid, currency_id, balance, updated_at FROM player_balances WHERE xuid = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);

        std::vector<PlayerBalance> result;
        while (stmt.executeStep()) {
            PlayerBalance balance;
            balance.xuid       = stmt.getColumn(0).getString();
            balance.currencyId = stmt.getColumn(1).getString();
            balance.balance    = stmt.getColumn(2).getInt();
            balance.updatedAt  = stmt.getColumn(3).getInt64();
            result.push_back(balance);
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取玩家所有余额失败: " + std::string(e.what()));
    }
}

bool PlayerDAO::initializeBalance(const std::string& xuid, const std::string& currencyId, int initialBalance) {
    try {
        auto& db = mDbManager.getConnection();

        // 检查是否已存在
        auto existing = getBalance(xuid, currencyId);
        if (existing.has_value()) {
            return true; // 已存在，无需初始化
        }

        // 创建新余额记录
        auto currentTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        const char* sql = "INSERT INTO player_balances (xuid, currency_id, balance, updated_at) VALUES (?, ?, ?, ?)";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);
        stmt.bind(2, currencyId);
        stmt.bind(3, initialBalance);
        stmt.bind(4, currentTime);

        stmt.exec();
        return true;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("初始化玩家余额失败: " + std::string(e.what()));
    }
}

bool PlayerDAO::updateUsername(const std::string& xuid, const std::string& newUsername) {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "UPDATE players SET username = ?, updated_at = ? WHERE xuid = ?";

        auto currentTime =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, newUsername);
        stmt.bind(2, currentTime);
        stmt.bind(3, xuid);

        stmt.exec();
        return stmt.getChanges() > 0;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("更新玩家用户名失败: " + std::string(e.what()));
    }
}

std::vector<TopBalanceEntry> PlayerDAO::getTopBalanceList(const std::string& currencyId, int limit) const {
    if (limit <= 0 || limit > 1000) {
        throw InvalidArgumentException("limit 必须在 1-1000 之间");
    }

    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT p.username, pb.xuid, pb.currency_id, pb.balance "
                          "FROM player_balances pb "
                          "INNER JOIN players p ON pb.xuid = p.xuid "
                          "WHERE pb.currency_id = ? "
                          "ORDER BY pb.balance DESC LIMIT ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, currencyId);
        stmt.bind(2, limit);

        std::vector<TopBalanceEntry> result;
        int                          rank = 1;
        while (stmt.executeStep()) {
            TopBalanceEntry entry;
            entry.username   = stmt.getColumn(0).getString();
            entry.xuid       = stmt.getColumn(1).getString();
            entry.currencyId = stmt.getColumn(2).getString();
            entry.balance    = stmt.getColumn(3).getInt();
            entry.rank       = rank++;
            result.push_back(entry);
        }

        return result;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取财富排行榜失败: " + std::string(e.what()));
    }
}

bool PlayerDAO::playerExists(const std::string& xuid) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT 1 FROM players WHERE xuid = ? LIMIT 1";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, xuid);

        return stmt.executeStep();

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("检查玩家是否存在失败: " + std::string(e.what()));
    }
}

int PlayerDAO::getPlayerCount() const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT COUNT(*) FROM players";

        SQLite::Statement stmt(db, sql);

        if (stmt.executeStep()) {
            return stmt.getColumn(0).getInt();
        }

        return 0;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取玩家总数失败: " + std::string(e.what()));
    }
}

int PlayerDAO::getTotalWealth(const std::string& currencyId) const {
    try {
        auto& db = mDbManager.getConnection();

        const char* sql = "SELECT SUM(balance) FROM player_balances WHERE currency_id = ?";

        SQLite::Statement stmt(db, sql);
        stmt.bind(1, currencyId);

        if (stmt.executeStep()) {
            return stmt.getColumn(0).getInt();
        }

        return 0;

    } catch (const SQLite::Exception& e) {
        throw DatabaseException("获取总财富失败: " + std::string(e.what()));
    }
}

PlayerData PlayerDAO::buildPlayerDataFromStatement(SQLite::Statement& stmt) const {
    PlayerData data;
    data.xuid          = stmt.getColumn(0).getString();
    data.username      = stmt.getColumn(1).getString();
    data.firstJoinTime = stmt.getColumn(2).getInt64();
    data.createdAt     = stmt.getColumn(3).getInt64();
    data.updatedAt     = stmt.getColumn(4).getInt64();
    return data;
}

} // namespace rlx_money