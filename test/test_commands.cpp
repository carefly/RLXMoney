#include "mocks/MockLeviLaminaAPI.h"
#include "mod/config/ConfigManager.h"
#include "mod/economy/EconomyManager.h"
#include "utils/CommandTestHelper.h"
#include "utils/TestTempManager.h"
#include <catch2/catch_all.hpp>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>


namespace {
// 使用测试临时文件管理器替代原来的 makeUniquePath
std::string makeUniquePath(const std::string& prefix, const std::string& extension) {
    return rlx_money::test::TestTempManager::getInstance().makeUniquePath(prefix, extension);
}

// 为每个 TEST_CASE 创建独立配置与数据库，并完成初始化
std::pair<std::string, std::string> setupIsolatedManager(
    const std::string& caseName,
    bool               walMode             = true,
    int                cacheSize           = 2000,
    const std::string& synchronous         = "NORMAL",
    int64_t            initialBalance      = 1000,
    int64_t            maxBalance          = 1000000,
    int64_t            minTransferAmount   = 1,
    int64_t            transferFee         = 0,
    double             feePercentage       = 0.0,
    bool               allowPlayerTransfer = true,
    int                defaultTopCount     = 10,
    int                maxTopCount         = 50
) {
    // 关闭之前的数据库连接，确保每个测试用例使用独立的数据库实例
    auto& dbManager = rlx_money::DatabaseManager::getInstance();
    if (dbManager.isInitialized()) {
        dbManager.close();
    }

    auto configPath = makeUniquePath("test_" + caseName, ".json");
    auto dbPath     = makeUniquePath("test_" + caseName, ".db");

    nlohmann::json testConfig;
    testConfig["database"]["path"]                        = dbPath;
    testConfig["database"]["optimization"]["wal_mode"]    = walMode;
    testConfig["database"]["optimization"]["cache_size"]  = cacheSize;
    testConfig["database"]["optimization"]["synchronous"] = synchronous;
    testConfig["defaultCurrency"]                         = "gold";

    // 创建默认币种配置
    testConfig["currencies"]["gold"]["name"]                = "金币";
    testConfig["currencies"]["gold"]["symbol"]              = "G";
    testConfig["currencies"]["gold"]["displayFormat"]       = "{amount} {symbol}";
    testConfig["currencies"]["gold"]["enabled"]             = true;
    testConfig["currencies"]["gold"]["initialBalance"]      = initialBalance;
    testConfig["currencies"]["gold"]["maxBalance"]          = maxBalance;
    testConfig["currencies"]["gold"]["minTransferAmount"]   = minTransferAmount;
    testConfig["currencies"]["gold"]["transferFee"]         = transferFee;
    testConfig["currencies"]["gold"]["feePercentage"]       = feePercentage;
    testConfig["currencies"]["gold"]["allowPlayerTransfer"] = allowPlayerTransfer;

    testConfig["top_list"]["default_count"] = defaultTopCount;
    testConfig["top_list"]["max_count"]     = maxTopCount;

    std::ofstream configFile(configPath);
    configFile << testConfig.dump(4);
    configFile.close();

    // 注册文件以便自动清理
    rlx_money::test::TestTempManager::getInstance().registerFile(configPath);
    rlx_money::test::TestTempManager::getInstance().registerFile(dbPath);

    auto& configManager = rlx_money::ConfigManager::getInstance();
    REQUIRE_NOTHROW(configManager.loadConfig(configPath));
    REQUIRE_NOTHROW(configManager.reloadConfig());

    auto& manager = rlx_money::EconomyManager::getInstance();
    REQUIRE(manager.initialize());

    return {configPath, dbPath};
}

void cleanupFiles(const std::vector<std::string>& paths) {
    for (const auto& p : paths) {
        std::remove(p.c_str());
    }
}

// 清理所有单例的初始化状态，确保测试之间隔离
void cleanupSingletonState() {
    // 重置 EconomyManager 的初始化状态
    auto& economyManager = rlx_money::EconomyManager::getInstance();
    economyManager.resetForTesting();

    // 清空数据库表（在关闭连接之前）
    auto& dbManager = rlx_money::DatabaseManager::getInstance();
    if (dbManager.isInitialized()) {
        try {
            (void)dbManager.executeTransaction([](SQLite::Database& db) {
                db.exec("DELETE FROM transactions;");
                db.exec("DELETE FROM player_balances;");
                db.exec("DELETE FROM players;");
                return true;
            });
        } catch (...) {
            // 忽略清空表时的异常，继续执行清理流程
        }
        // 关闭数据库连接
        dbManager.close();
    }

    // 重置 ConfigManager 的配置状态
    auto& configManager = rlx_money::ConfigManager::getInstance();
    configManager.resetForTesting();

    // 清理 Mock 玩家数据
    rlx_money::LeviLaminaAPI::clearMockPlayers();
}

// RAII 清理守卫
class SingletonCleanupGuard {
public:
    ~SingletonCleanupGuard() { cleanupSingletonState(); }
};
} // namespace

// ============================================================================
// 命令测试用例
// ============================================================================

TEST_CASE("命令测试 - BasicCommand 查询余额", "[commands][basic][query]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_basic_query");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("查询默认币种余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("test123", "testplayer");

        // 初始化玩家
        manager.initializeNewPlayer("test123", "testplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("test123", currencyId, 5000);

        // 测试查询命令
        REQUIRE(rlx_money::test::CommandTestHelper::testBasicQueryCommand("test123", "testplayer", currencyId, true));
    }

    SECTION("查询所有币种余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("test456", "testplayer2");

        manager.initializeNewPlayer("test456", "testplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("test456", currencyId, 3000);

        // 测试查询所有币种（不指定币种）
        REQUIRE(rlx_money::test::CommandTestHelper::testBasicQueryCommand(
            "test456",
            "testplayer2",
            "", // 空币种表示查询所有
            true
        ));
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - BasicCommand 查询历史", "[commands][basic][history]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_basic_history");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("查询交易历史") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("hist123", "histplayer");

        manager.initializeNewPlayer("hist123", "histplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("hist123", currencyId, 1000);
        manager.addMoney("hist123", currencyId, 500, "测试增加");

        // 测试查询历史命令
        REQUIRE(rlx_money::test::CommandTestHelper::testBasicHistoryCommand("hist123", "histplayer", currencyId));
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - PayCommand 转账", "[commands][pay]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_pay");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("正常转账") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("from123", "fromplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("to123", "toplayer");

        manager.initializeNewPlayer("from123", "fromplayer");
        manager.initializeNewPlayer("to123", "toplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("from123", currencyId, 1000);
        manager.setBalance("to123", currencyId, 500);

        // 测试转账命令
        REQUIRE(rlx_money::test::CommandTestHelper::testPayCommand(
            "from123",
            "fromplayer",
            "toplayer",
            300,
            currencyId,
            true
        ));

        // 验证转账结果
        auto fromBalance = manager.getBalance("from123", currencyId);
        auto toBalance   = manager.getBalance("to123", currencyId);
        REQUIRE(fromBalance.value() == 700);
        REQUIRE(toBalance.value() == 800);
    }

    SECTION("转账金额无效") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("from456", "fromplayer2");
        rlx_money::LeviLaminaAPI::addMockPlayer("to456", "toplayer2");

        manager.initializeNewPlayer("from456", "fromplayer2");
        manager.initializeNewPlayer("to456", "toplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("from456", currencyId, 1000);

        // 测试转账金额为0应该失败（期望失败，所以 expectSuccess=false，应该返回 true）
        REQUIRE(rlx_money::test::CommandTestHelper::testPayCommand(
            "from456",
            "fromplayer2",
            "toplayer2",
            0,
            currencyId,
            false
        ));
    }

    SECTION("转账金额不足") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("from789", "fromplayer3");
        rlx_money::LeviLaminaAPI::addMockPlayer("to789", "toplayer3");

        manager.initializeNewPlayer("from789", "fromplayer3");
        manager.initializeNewPlayer("to789", "toplayer3");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("from789", currencyId, 100);

        // 测试转账金额超过余额应该失败
        REQUIRE(rlx_money::test::CommandTestHelper::testPayCommand(
            "from789",
            "fromplayer3",
            "toplayer3",
            200,
            currencyId,
            false
        ));
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - AdminCommand 设置余额", "[commands][admin][set]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_admin_set");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("设置余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin123", "adminplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("target123", "targetplayer");

        manager.initializeNewPlayer("admin123", "adminplayer");
        manager.initializeNewPlayer("target123", "targetplayer");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 测试设置余额命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminSetCommand(
            "admin123",
            "adminplayer",
            "targetplayer",
            5000,
            currencyId,
            true
        ));

        // 验证余额已设置
        auto balance = manager.getBalance("target123", currencyId);
        REQUIRE(balance.value() == 5000);
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - AdminCommand 给予和扣除", "[commands][admin][give][take]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_admin_give_take");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("给予金钱") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin456", "adminplayer2");
        rlx_money::LeviLaminaAPI::addMockPlayer("target456", "targetplayer2");

        manager.initializeNewPlayer("admin456", "adminplayer2");
        manager.initializeNewPlayer("target456", "targetplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("target456", currencyId, 1000);

        // 测试给予命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminGiveCommand(
            "admin456",
            "adminplayer2",
            "targetplayer2",
            500,
            currencyId,
            true
        ));

        // 验证余额增加
        auto balance = manager.getBalance("target456", currencyId);
        REQUIRE(balance.value() == 1500);
    }

    SECTION("扣除金钱") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin789", "adminplayer3");
        rlx_money::LeviLaminaAPI::addMockPlayer("target789", "targetplayer3");

        manager.initializeNewPlayer("admin789", "adminplayer3");
        manager.initializeNewPlayer("target789", "targetplayer3");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("target789", currencyId, 1000);

        // 测试扣除命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminTakeCommand(
            "admin789",
            "adminplayer3",
            "targetplayer3",
            300,
            currencyId,
            true
        ));

        // 验证余额减少
        auto balance = manager.getBalance("target789", currencyId);
        REQUIRE(balance.value() == 700);
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - AdminCommand 查询和排行榜", "[commands][admin][check][top]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_admin_check_top");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("查询余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin999", "adminplayer4");
        rlx_money::LeviLaminaAPI::addMockPlayer("target999", "targetplayer4");

        manager.initializeNewPlayer("admin999", "adminplayer4");
        manager.initializeNewPlayer("target999", "targetplayer4");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("target999", currencyId, 2000);

        // 测试查询命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminCheckCommand(
            "admin999",
            "adminplayer4",
            "targetplayer4",
            currencyId
        ));
    }

    SECTION("排行榜") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin000", "adminplayer5");
        rlx_money::LeviLaminaAPI::addMockPlayer("top1", "topplayer1");
        rlx_money::LeviLaminaAPI::addMockPlayer("top2", "topplayer2");
        rlx_money::LeviLaminaAPI::addMockPlayer("top3", "topplayer3");

        manager.initializeNewPlayer("admin000", "adminplayer5");
        manager.initializeNewPlayer("top1", "topplayer1");
        manager.initializeNewPlayer("top2", "topplayer2");
        manager.initializeNewPlayer("top3", "topplayer3");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 设置不同玩家的余额，用于测试排行榜
        manager.setBalance("top1", currencyId, 5000);
        manager.setBalance("top2", currencyId, 3000);
        manager.setBalance("top3", currencyId, 1000);

        // 测试排行榜命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminTopCommand("admin000", "adminplayer5", currencyId));
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - CurrencyCommand 币种管理", "[commands][currency]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_currency");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("币种列表") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin111", "adminplayer6");

        // 测试币种列表命令
        REQUIRE(rlx_money::test::CommandTestHelper::testCurrencyListCommand("admin111", "adminplayer6"));
    }

    SECTION("币种信息") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin222", "adminplayer7");

        std::string currencyId = manager.getDefaultCurrencyId();

        // 测试币种信息命令
        REQUIRE(
            rlx_money::test::CommandTestHelper::testCurrencyInfoCommand("admin222", "adminplayer7", currencyId, true)
        );
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - AdminCommand 历史记录", "[commands][admin][history]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("commands_admin_history");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("查询玩家历史记录") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin333", "adminplayer8");
        rlx_money::LeviLaminaAPI::addMockPlayer("target333", "targetplayer8");

        manager.initializeNewPlayer("admin333", "adminplayer8");
        manager.initializeNewPlayer("target333", "targetplayer8");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("target333", currencyId, 1000);
        manager.addMoney("target333", currencyId, 500, "测试增加");
        manager.reduceMoney("target333", currencyId, 200, "测试扣除");

        // 测试查询历史记录命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminHistoryCommand(
            "admin333",
            "adminplayer8",
            "targetplayer8",
            currencyId
        ));
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - AdminCommand 初始金额管理", "[commands][admin][initial]") {
    auto cleanupGuard = SingletonCleanupGuard{};
    auto paths        = setupIsolatedManager("commands_admin_initial");

    SECTION("设置初始金额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin444", "adminplayer9");

        // 测试设置初始金额命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminSetInitialCommand("admin444", "adminplayer9", 2000, true));
    }

    SECTION("获取初始金额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin555", "adminplayer10");

        // 测试获取初始金额命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminGetInitialCommand("admin555", "adminplayer10"));
    }

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("命令测试 - AdminCommand 重载配置", "[commands][admin][reload]") {
    auto cleanupGuard = SingletonCleanupGuard{};
    auto paths        = setupIsolatedManager("commands_admin_reload");

    SECTION("重载配置") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("admin666", "adminplayer11");

        // 测试重载配置命令
        REQUIRE(rlx_money::test::CommandTestHelper::testAdminReloadCommand("admin666", "adminplayer11"));
    }

    cleanupFiles({paths.first, paths.second});
}
