#include "mod/dao/PlayerDAO.h"
#include "mod/dao/TransactionDAO.h"
#include "mod/data/DataStructures.h"
#include "mod/database/DatabaseManager.h"
#include "utils/TestTempManager.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <filesystem>


// ============================================================================
// 数据库管理器测试
// ============================================================================

TEST_CASE("DatabaseManager 测试", "[database][manager]") {
    // 使用临时文件管理器创建测试数据库路径
    auto& tempManager = rlx_money::test::TestTempManager::getInstance();
    const std::string testDbPath = tempManager.makeUniquePath("test_money", ".db");

    // 注册文件以便自动清理
    tempManager.registerFile(testDbPath);

    SECTION("单例模式测试") {
        auto& manager1 = rlx_money::DatabaseManager::getInstance();
        auto& manager2 = rlx_money::DatabaseManager::getInstance();

        REQUIRE(&manager1 == &manager2);
    }

    SECTION("数据库初始化测试") {
        auto& manager = rlx_money::DatabaseManager::getInstance();

        // 初始化数据库
        REQUIRE(manager.initialize(testDbPath));

        // 验证数据库文件是否创建
        REQUIRE(std::filesystem::exists(testDbPath));

        // 重复初始化应该成功
        REQUIRE(manager.initialize(testDbPath));

        // 关闭连接（文件会自动清理）
        manager.close();
    }

    SECTION("数据库连接测试") {
        auto& manager = rlx_money::DatabaseManager::getInstance();

        // 未初始化时检查连接状态
        // 这取决于实现，可能返回false或抛出异常

        // 初始化后检查连接状态
        REQUIRE(manager.initialize(testDbPath));
        // manager.isConnected(); // 如果有此方法的话

        // 关闭连接（文件会自动清理）
        manager.close();
    }
}

// ============================================================================
// PlayerDAO 测试
// ============================================================================

TEST_CASE("PlayerDAO 测试", "[database][dao][player]") {
    // 使用临时文件管理器创建测试数据库路径
    auto& tempManager = rlx_money::test::TestTempManager::getInstance();
    const std::string testDbPath = tempManager.makeUniquePath("test_player_dao", ".db");

    // 注册文件以便自动清理
    tempManager.registerFile(testDbPath);

    SECTION("玩家数据插入和查询") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::PlayerDAO playerDAO(dbManager);

        // 插入新玩家（不再包含balance字段）
        rlx_money::PlayerData newPlayer("12345", "testplayer", 1600000000);
        REQUIRE(playerDAO.createPlayer(newPlayer));

        // 查询玩家数据
        auto playerData = playerDAO.getPlayerByXuid("12345");
        REQUIRE(playerData.has_value());
        REQUIRE(playerData->xuid == "12345");
        REQUIRE(playerData->username == "testplayer");
        REQUIRE(playerData->firstJoinTime == 1600000000);

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("玩家数据更新") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::PlayerDAO playerDAO(dbManager);

        // 插入玩家（不再包含balance字段）
        rlx_money::PlayerData player("12345", "testplayer", 1600000000);
        REQUIRE(playerDAO.createPlayer(player));

        // 更新余额（需要指定币种）
        REQUIRE(playerDAO.updateBalance("12345", "gold", 2000));

        // 验证更新
        auto updatedPlayer = playerDAO.getPlayerByXuid("12345");
        REQUIRE(updatedPlayer.has_value());
        auto balance = playerDAO.getBalance("12345", "gold");
        REQUIRE(balance.has_value());
        REQUIRE(balance.value() == 2000);

        // 更新用户名
        REQUIRE(playerDAO.updateUsername("12345", "newplayername"));

        // 验证用户名更新
        auto renamedPlayer = playerDAO.getPlayerByXuid("12345");
        REQUIRE(renamedPlayer.has_value());
        REQUIRE(renamedPlayer->username == "newplayername");

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("玩家存在性检查") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::PlayerDAO playerDAO(dbManager);

        // 检查不存在的玩家
        REQUIRE_FALSE(playerDAO.playerExists("nonexistent"));

        // 插入玩家（不再包含balance字段）
        rlx_money::PlayerData player("12345", "testplayer", 1600000000);
        REQUIRE(playerDAO.createPlayer(player));

        // 检查存在的玩家
        REQUIRE(playerDAO.playerExists("12345"));

        // 清理
        dbManager.close();
        // 文件会自动清理
    }


    SECTION("统计信息查询") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::PlayerDAO playerDAO(dbManager);

        // 插入多个玩家（不再包含balance字段）
        std::vector<rlx_money::PlayerData> players = {
            {"111", "player1", 1600000000},
            {"222", "player2", 1600000001},
            {"333", "player3", 1600000002}
        };

        for (const auto& p : players) {
            REQUIRE(playerDAO.createPlayer(p));
        }

        // 设置余额（需要指定币种）
        playerDAO.updateBalance("111", "gold", 1000);
        playerDAO.updateBalance("222", "gold", 2000);
        playerDAO.updateBalance("333", "gold", 3000);

        // 获取玩家总数
        int playerCount = playerDAO.getPlayerCount();
        REQUIRE(playerCount == 3);

        // 获取总财富（需要指定币种）
        int64_t totalWealth = playerDAO.getTotalWealth("gold");
        REQUIRE(totalWealth == 6000); // 1000 + 2000 + 3000

        // 清理
        dbManager.close();
        // 文件会自动清理
    }
}

// ============================================================================
// TransactionDAO 测试
// ============================================================================

TEST_CASE("TransactionDAO 测试", "[database][dao][transaction]") {
    // 使用临时文件管理器创建测试数据库路径
    auto& tempManager = rlx_money::test::TestTempManager::getInstance();
    const std::string testDbPath = tempManager.makeUniquePath("test_transaction_dao", ".db");

    // 注册文件以便自动清理（WAL和SHM文件会自动注册）
    tempManager.registerFile(testDbPath);

    // 文件会自动清理

    SECTION("交易记录插入和查询") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 插入交易记录（需要包含currencyId）
        rlx_money::TransactionRecord
            record(1, "12345", "gold", 500, 1500, rlx_money::TransactionType::ADD, "测试交易", 1600000000);
        REQUIRE(transactionDAO.createTransaction(record));

        // 查询交易记录
        auto transactions = transactionDAO.getPlayerTransactions("12345", "", 1, 10);
        REQUIRE(transactions.size() == 1);
        REQUIRE(transactions[0].xuid == "12345");
        REQUIRE(transactions[0].amount == 500);
        REQUIRE(transactions[0].balance == 1500);
        REQUIRE(transactions[0].type == rlx_money::TransactionType::ADD);
        REQUIRE(transactions[0].description == "测试交易");

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("交易记录分页查询") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 插入多个交易记录（需要包含currencyId）
        for (int i = 1; i <= 25; ++i) {
            rlx_money::TransactionRecord record(

                i,
                "12345",
                "gold",
                i * 100,
                i * 1000,
                rlx_money::TransactionType::ADD,
                "交易 " + std::to_string(i),
                1600000000 + i
            );
            REQUIRE(transactionDAO.createTransaction(record));
        }

        // 测试分页
        auto page1 = transactionDAO.getPlayerTransactions("12345", "", 1, 10);
        REQUIRE(page1.size() == 10);

        auto page2 = transactionDAO.getPlayerTransactions("12345", "", 2, 10);
        REQUIRE(page2.size() == 10);

        auto page3 = transactionDAO.getPlayerTransactions("12345", "", 3, 10);
        REQUIRE(page3.size() == 5);

        // 超出范围的页面
        auto emptyPage = transactionDAO.getPlayerTransactions("12345", "", 4, 10);
        REQUIRE(emptyPage.empty());

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("交易记录计数") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 插入交易记录（需要包含currencyId）
        for (int i = 1; i <= 15; ++i) {
            rlx_money::TransactionRecord record(
                i,
                "12345",
                "gold",
                100,
                1000 + i * 100,
                rlx_money::TransactionType::ADD,
                "交易 " + std::to_string(i),
                1600000000 + i
            );
            REQUIRE(transactionDAO.createTransaction(record));
        }

        // 获取交易记录总数
        int count = transactionDAO.getPlayerTransactionCount("12345");
        REQUIRE(count == 15);

        // 不存在玩家的交易记录计数
        int emptyCount = transactionDAO.getPlayerTransactionCount("nonexistent");
        REQUIRE(emptyCount == 0);

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("按交易类型查询") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 插入不同类型的交易记录（需要包含currencyId）
        rlx_money::TransactionRecord
            record1(1, "12345", "gold", 1000, 1000, rlx_money::TransactionType::SET, "设置余额", 1600000000);
        rlx_money::TransactionRecord
            record2(2, "12345", "gold", 500, 1500, rlx_money::TransactionType::ADD, "增加金钱", 1600000001);
        rlx_money::TransactionRecord
            record3(3, "12345", "gold", -200, 1300, rlx_money::TransactionType::REDUCE, "扣除金钱", 1600000002);

        REQUIRE(transactionDAO.createTransaction(record1));
        REQUIRE(transactionDAO.createTransaction(record2));
        REQUIRE(transactionDAO.createTransaction(record3));

        // 按类型查询
        auto addTransactions =
            transactionDAO.getPlayerTransactionsByType("12345", rlx_money::TransactionType::ADD, 1, 10);
        REQUIRE(addTransactions.size() == 1);
        REQUIRE(addTransactions[0].type == rlx_money::TransactionType::ADD);

        auto reduceTransactions =
            transactionDAO.getPlayerTransactionsByType("12345", rlx_money::TransactionType::REDUCE, 1, 10);
        REQUIRE(reduceTransactions.size() == 1);
        REQUIRE(reduceTransactions[0].type == rlx_money::TransactionType::REDUCE);

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("转账交易记录") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 插入转账交易记录（需要包含currencyId）
        rlx_money::TransactionRecord fromRecord(
            1,
            "12345",
            "gold",
            -300,
            700,
            rlx_money::TransactionType::TRANSFER,
            "转账给 player2",
            1600000000,
            "67890"
        );

        rlx_money::TransactionRecord toRecord(
            2,
            "67890",
            "gold",
            300,
            1300,
            rlx_money::TransactionType::TRANSFER,
            "从 player1 转账",
            1600000000,
            "12345"
        );

        REQUIRE(transactionDAO.createTransaction(fromRecord));
        REQUIRE(transactionDAO.createTransaction(toRecord));

        // 查询转账记录
        auto fromTransactions = transactionDAO.getPlayerTransactions("12345", "", 1, 10);
        REQUIRE(fromTransactions.size() == 1);
        REQUIRE(fromTransactions[0].relatedXuid.has_value());
        REQUIRE(fromTransactions[0].relatedXuid.value() == "67890");

        auto toTransactions = transactionDAO.getPlayerTransactions("67890", "", 1, 10);
        REQUIRE(toTransactions.size() == 1);
        REQUIRE(toTransactions[0].relatedXuid.has_value());
        REQUIRE(toTransactions[0].relatedXuid.value() == "12345");

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("交易记录清理") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::TransactionDAO transactionDAO(dbManager);

        const int64_t nowSec =
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
                .count();
        const int64_t thirtyDays = 30LL * 24 * 60 * 60;

        // 插入旧交易记录（31天前，需要包含currencyId）
        rlx_money::TransactionRecord oldRecord(
            1,
            "12345",
            "gold",
            100,
            100,
            rlx_money::TransactionType::ADD,
            "旧交易",
            nowSec - (thirtyDays + 24 * 60 * 60)
        );
        REQUIRE(transactionDAO.createTransaction(oldRecord));

        // 插入新交易记录（1天前，需要包含currencyId）
        rlx_money::TransactionRecord
            newRecord(2, "12345", "gold", 200, 300, rlx_money::TransactionType::ADD, "新交易", nowSec - (24 * 60 * 60));
        REQUIRE(transactionDAO.createTransaction(newRecord));

        // 清理旧记录（保留最近30天的记录）
        int deletedCount = transactionDAO.cleanupOldTransactions(30);
        REQUIRE(deletedCount == 1); // 应该删除1条旧记录

        // 验证只剩新记录
        auto remainingTransactions = transactionDAO.getPlayerTransactions("12345", "", 1, 10);
        REQUIRE(remainingTransactions.size() == 1);
        REQUIRE(remainingTransactions[0].description == "新交易");

        // 清理
        dbManager.close();
        // 文件会自动清理
    }
}

// ============================================================================
// 数据库事务测试
// ============================================================================

TEST_CASE("数据库事务测试", "[database][transaction]") {
    // 使用临时文件管理器创建测试数据库路径
    auto& tempManager = rlx_money::test::TestTempManager::getInstance();
    const std::string testDbPath = tempManager.makeUniquePath("test_transaction", ".db");

    // 注册文件以便自动清理
    tempManager.registerFile(testDbPath);

    // 文件会自动清理

    SECTION("成功事务") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::PlayerDAO      playerDAO(dbManager);
        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 执行事务
        bool success = dbManager.executeTransaction([&playerDAO, &transactionDAO](SQLite::Database&) -> bool {
            // 在事务中执行操作（不再包含balance字段）
            rlx_money::PlayerData player("12345", "testplayer", 1600000000);
            if (!playerDAO.createPlayer(player)) {
                return false;
            }

            rlx_money::TransactionRecord
                record(1, "12345", "gold", 1000, 1000, rlx_money::TransactionType::INITIAL, "初始余额", 1600000000);
            if (!transactionDAO.createTransaction(record)) {
                return false;
            }

            return true;
        });

        REQUIRE(success);

        // 验证数据已持久化
        REQUIRE(playerDAO.playerExists("12345"));
        auto transactions = transactionDAO.getPlayerTransactions("12345", "", 1, 10);
        REQUIRE(transactions.size() == 1);

        // 清理
        dbManager.close();
        // 文件会自动清理
    }

    SECTION("回滚事务") {
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        REQUIRE(dbManager.initialize(testDbPath));

        rlx_money::PlayerDAO      playerDAO(dbManager);
        rlx_money::TransactionDAO transactionDAO(dbManager);

        // 执行失败的事务（模拟回滚）
        bool success = dbManager.executeTransaction([&playerDAO](SQLite::Database&) -> bool {
            // 在事务中执行操作（不再包含balance字段）
            rlx_money::PlayerData player("12345", "testplayer", 1600000000);
            if (!playerDAO.createPlayer(player)) {
                return false;
            }

            // 故意返回false来触发回滚
            return false;
        });

        REQUIRE_FALSE(success);

        // 验证数据未持久化
        REQUIRE_FALSE(playerDAO.playerExists("12345"));
        auto transactions = transactionDAO.getPlayerTransactions("12345", "", 1, 10);
        REQUIRE(transactions.empty());

        // 清理
        dbManager.close();
        // 文件会自动清理
    }
}