#include "mocks/MockLeviLaminaAPI.h"
#include "mod/config/ConfigManager.h"
#include "mod/core/SystemInitializer.h"
#include "mod/data/DataStructures.h"
#include "mod/database/DatabaseManager.h"
#include "mod/economy/EconomyManager.h"
#include "mod/types/Types.h"
#include "utils/TestTempManager.h"
#include <catch2/catch_all.hpp>
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

    // 创建默认币种配置（所有字段都在 currencies["gold"] 节点下，使用驼峰命名）
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

// 确保数据库和 EconomyManager 已初始化（用于 SECTION 中）
void ensureDatabaseInitialized() {
    auto& dbManager = rlx_money::DatabaseManager::getInstance();
    if (!dbManager.isInitialized()) {
        auto& config = rlx_money::ConfigManager::getInstance().getConfig();
        REQUIRE(dbManager.initialize(config.database.path));
    }
    auto& manager = rlx_money::EconomyManager::getInstance();
    REQUIRE(manager.initialize());
}

// 清空数据库中的玩家与交易表，确保排行榜空数据场景
void truncateAllTables() {
    auto& dbm = rlx_money::DatabaseManager::getInstance();
    (void)dbm.executeTransaction([](SQLite::Database& db) {
        db.exec("DELETE FROM transactions;");
        db.exec("DELETE FROM player_balances;");
        db.exec("DELETE FROM players;");
        return true;
    });
}

// 清理所有单例的初始化状态，确保测试之间隔离
void cleanupSingletonState() {
    try {
        // 先清空数据库表（在重置单例之前）
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        if (dbManager.isInitialized()) {
            try {
                truncateAllTables();
            } catch (...) {
                // 忽略清空表时的异常，继续执行清理流程
            }
        }

        // 使用 SystemInitializer 统一重置所有单例
        rlx_money::SystemInitializer::resetAllForTesting();
    } catch (...) {
        // 如果 SystemInitializer 重置失败，使用手动方式清理
        // 重置 EconomyManager 的初始化状态
        auto& economyManager = rlx_money::EconomyManager::getInstance();
        economyManager.resetForTesting();

        // 清空数据库表（在关闭连接之前）
        auto& dbManager = rlx_money::DatabaseManager::getInstance();
        if (dbManager.isInitialized()) {
            try {
                truncateAllTables();
            } catch (...) {
                // 忽略清空表时的异常，继续执行清理流程
            }
            // 关闭数据库连接
            dbManager.close();
        }

        // 重置 ConfigManager 的配置状态
        auto& configManager = rlx_money::ConfigManager::getInstance();
        configManager.resetForTesting();
    }

    // 清理 Mock 玩家数据
    rlx_money::LeviLaminaAPI::clearMockPlayers();
}

// RAII 清理守卫：在测试用例结束时自动清理单例状态
// 使用方法：在每个 TEST_CASE 开始时声明 `auto cleanupGuard = SingletonCleanupGuard{};`
class SingletonCleanupGuard {
public:
    ~SingletonCleanupGuard() { cleanupSingletonState(); }
};
} // namespace

// 测试LeviLamina API的Mock功能
TEST_CASE("经济管理器 - Mock 测试", "[economy][mock][levilamina]") {
    auto cleanupGuard = SingletonCleanupGuard{};

    SECTION("初始化和清理") {
        // 在每个测试前清理数据
        rlx_money::LeviLaminaAPI::clearMockPlayers();

        // 验证清理后没有玩家数据
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByXuid("12345") == nullptr);
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByName("testplayer") == nullptr);
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerNameByXuid("12345").empty());
        REQUIRE(rlx_money::LeviLaminaAPI::getXuidByPlayerName("testplayer").empty());
    }

    SECTION("添加和获取玩家") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();

        // 添加测试玩家
        rlx_money::LeviLaminaAPI::addMockPlayer("12345", "testplayer");

        // 测试通过XUID获取玩家
        Player* playerByXuid = rlx_money::LeviLaminaAPI::getPlayerByXuid("12345");
        REQUIRE(playerByXuid != nullptr);
        REQUIRE(playerByXuid->getXuid() == "12345");
        REQUIRE(playerByXuid->name == "testplayer");

        // 测试通过名称获取玩家
        Player* playerByName = rlx_money::LeviLaminaAPI::getPlayerByName("testplayer");
        REQUIRE(playerByName != nullptr);
        REQUIRE(playerByName->getXuid() == "12345");
        REQUIRE(playerByName->name == "testplayer");

        // 验证是同一个对象
        REQUIRE(playerByXuid == playerByName);
    }

    SECTION("玩家名称和XUID转换") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();

        // 添加测试玩家
        rlx_money::LeviLaminaAPI::addMockPlayer("67890", "anotherplayer");

        // 测试名称和XUID转换
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerNameByXuid("67890") == "anotherplayer");
        REQUIRE(rlx_money::LeviLaminaAPI::getXuidByPlayerName("anotherplayer") == "67890");

        // 测试不存在的玩家
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerNameByXuid("99999").empty());
        REQUIRE(rlx_money::LeviLaminaAPI::getXuidByPlayerName("nonexistent").empty());
    }

    SECTION("多个玩家管理") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();

        // 添加多个测试玩家
        rlx_money::LeviLaminaAPI::addMockPlayer("111", "player1");
        rlx_money::LeviLaminaAPI::addMockPlayer("222", "player2");
        rlx_money::LeviLaminaAPI::addMockPlayer("333", "player3");

        // 验证所有玩家都能正确获取
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByXuid("111") != nullptr);
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByXuid("222") != nullptr);
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByXuid("333") != nullptr);

        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByName("player1") != nullptr);
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByName("player2") != nullptr);
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerByName("player3") != nullptr);

        // 验证名称和XUID转换
        REQUIRE(rlx_money::LeviLaminaAPI::getPlayerNameByXuid("111") == "player1");
        REQUIRE(rlx_money::LeviLaminaAPI::getXuidByPlayerName("player2") == "222");
    }
}

// ============================================================================
// EconomyManager 测试用例
// ============================================================================

TEST_CASE("EconomyManager 初始化测试", "[economy][manager][init]") {
    auto cleanupGuard = SingletonCleanupGuard{};
    // 为测试设置临时配置
    auto& tempManager   = rlx_money::test::TestTempManager::getInstance();
    auto& configManager = rlx_money::ConfigManager::getInstance();

    std::string testConfigPath = tempManager.makeUniquePath("test_economy_config", ".json");
    std::string testDbPath     = tempManager.makeUniquePath("test_economy_manager", ".db");

    // 注册文件以便自动清理
    tempManager.registerFile(testConfigPath);
    tempManager.registerFile(testDbPath);

    // 创建测试配置（使用新的配置格式，所有字段都在 currencies["gold"] 节点下，使用驼峰命名）
    nlohmann::json testConfig;
    testConfig["database"]["path"]                        = testDbPath;
    testConfig["database"]["optimization"]["wal_mode"]    = true;
    testConfig["database"]["optimization"]["cache_size"]  = 2000;
    testConfig["database"]["optimization"]["synchronous"] = "NORMAL";
    testConfig["defaultCurrency"]                         = "gold";

    // 创建默认币种配置
    testConfig["currencies"]["gold"]["name"]                = "金币";
    testConfig["currencies"]["gold"]["symbol"]              = "G";
    testConfig["currencies"]["gold"]["displayFormat"]       = "{amount} {symbol}";
    testConfig["currencies"]["gold"]["enabled"]             = true;
    testConfig["currencies"]["gold"]["initialBalance"]      = 1000;
    testConfig["currencies"]["gold"]["maxBalance"]          = 1000000;
    testConfig["currencies"]["gold"]["minTransferAmount"]   = 1;
    testConfig["currencies"]["gold"]["transferFee"]         = 0;
    testConfig["currencies"]["gold"]["feePercentage"]       = 0.0;
    testConfig["currencies"]["gold"]["allowPlayerTransfer"] = true;

    testConfig["top_list"]["default_count"] = 10;
    testConfig["top_list"]["max_count"]     = 50;

    std::ofstream configFile(testConfigPath);
    configFile << testConfig.dump(4);
    configFile.close();

    // 先加载配置设置路径，然后重新加载配置
    REQUIRE_NOTHROW(configManager.loadConfig(testConfigPath));
    REQUIRE_NOTHROW(configManager.reloadConfig());

    auto& manager = rlx_money::EconomyManager::getInstance();

    SECTION("单例模式测试") {
        auto& manager1 = rlx_money::EconomyManager::getInstance();
        auto& manager2 = rlx_money::EconomyManager::getInstance();

        REQUIRE(&manager1 == &manager2);
    }

    SECTION("初始化功能测试") {
        // 测试初始化
        REQUIRE(manager.initialize());

        // 重复初始化应该返回成功
        REQUIRE(manager.initialize());
    }

    // 清理测试文件
    std::remove(testConfigPath.c_str());
    std::remove(testDbPath.c_str());
}

TEST_CASE("EconomyManager 玩家管理测试", "[economy][manager][player]") {
    auto cleanupGuard = SingletonCleanupGuard{};
    // 关闭之前的数据库连接，确保每个测试用例使用独立的数据库实例
    auto& dbManager = rlx_money::DatabaseManager::getInstance();
    if (dbManager.isInitialized()) {
        dbManager.close();
    }

    // 为测试设置临时配置
    auto& tempManager   = rlx_money::test::TestTempManager::getInstance();
    auto& configManager = rlx_money::ConfigManager::getInstance();

    std::string testConfigPath = tempManager.makeUniquePath("test_economy_player_config", ".json");
    std::string testDbPath     = tempManager.makeUniquePath("test_economy_player", ".db");

    // 注册文件以便自动清理
    tempManager.registerFile(testConfigPath);
    tempManager.registerFile(testDbPath);

    // 创建测试配置（使用新的配置格式，所有字段都在 currencies["gold"] 节点下）
    nlohmann::json testConfig;
    testConfig["database"]["path"]                        = testDbPath;
    testConfig["database"]["optimization"]["wal_mode"]    = true;
    testConfig["database"]["optimization"]["cache_size"]  = 2000;
    testConfig["database"]["optimization"]["synchronous"] = "NORMAL";
    testConfig["defaultCurrency"]                         = "gold";

    // 创建默认币种配置
    testConfig["currencies"]["gold"]["name"]                = "金币";
    testConfig["currencies"]["gold"]["symbol"]              = "G";
    testConfig["currencies"]["gold"]["displayFormat"]       = "{amount} {symbol}";
    testConfig["currencies"]["gold"]["enabled"]             = true;
    testConfig["currencies"]["gold"]["initialBalance"]      = 1000;
    testConfig["currencies"]["gold"]["maxBalance"]          = 1000000;
    testConfig["currencies"]["gold"]["minTransferAmount"]   = 1;
    testConfig["currencies"]["gold"]["transferFee"]         = 0;
    testConfig["currencies"]["gold"]["feePercentage"]       = 0.0;
    testConfig["currencies"]["gold"]["allowPlayerTransfer"] = true;

    testConfig["top_list"]["default_count"] = 10;
    testConfig["top_list"]["max_count"]     = 50;

    std::ofstream configFile(testConfigPath);
    configFile << testConfig.dump(4);
    configFile.close();

    // 先加载配置设置路径，然后重新加载配置
    REQUIRE_NOTHROW(configManager.loadConfig(testConfigPath));
    REQUIRE_NOTHROW(configManager.reloadConfig());

    auto& manager = rlx_money::EconomyManager::getInstance();
    REQUIRE(manager.initialize());

    SECTION("玩家存在性检查") {
        // 清理数据库表和 Mock 玩家
        truncateAllTables();
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("test123", "testplayer");

        // 初始化新玩家
        REQUIRE(manager.initializeNewPlayer("test123", "testplayer"));

        // 检查玩家存在
        REQUIRE(manager.playerExists("test123"));

        // 检查不存在的玩家
        REQUIRE_FALSE(manager.playerExists("nonexistent"));
    }

    SECTION("新玩家初始化") {
        // 清理数据库表和 Mock 玩家
        truncateAllTables();
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("new123", "newplayer");

        // 初始化新玩家
        REQUIRE(manager.initializeNewPlayer("new123", "newplayer"));

        // 验证初始余额
        std::string currencyId = manager.getDefaultCurrencyId();
        auto        balance    = manager.getBalance("new123", currencyId);
        REQUIRE(balance.has_value());
        REQUIRE(balance.value() >= 0); // 初始余额应该 >= 0
    }

    SECTION("重复初始化玩家") {
        // 清理数据库表和 Mock 玩家
        truncateAllTables();
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("dup123", "dupplayer");

        // 第一次初始化
        REQUIRE(manager.initializeNewPlayer("dup123", "dupplayer"));

        // 第二次初始化应抛出玩家已存在异常
        REQUIRE_THROWS(manager.initializeNewPlayer("dup123", "dupplayer"));
    }

    // 清理测试文件
    std::remove(testConfigPath.c_str());
    std::remove(testDbPath.c_str());
}

TEST_CASE("EconomyManager 余额操作测试", "[economy][manager][balance]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("economy_balance");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("获取余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("bal123", "balplayer");

        // 初始化玩家
        manager.initializeNewPlayer("bal123", "balplayer");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 获取余额
        auto balance = manager.getBalance("bal123", currencyId);
        REQUIRE(balance.has_value());
        REQUIRE(balance.value() >= 0);

        // 不存在的玩家余额
        auto noBalance = manager.getBalance("nonexistent", currencyId);
        REQUIRE_FALSE(noBalance.has_value());
    }

    SECTION("设置余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("set123", "setplayer");

        manager.initializeNewPlayer("set123", "setplayer");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 设置正数余额
        REQUIRE(manager.setBalance("set123", currencyId, 1000, "测试设置"));
        auto balance = manager.getBalance("set123", currencyId);
        REQUIRE(balance.value() == 1000);

        // 设置零余额
        REQUIRE(manager.setBalance("set123", currencyId, 0, "清零余额"));
        balance = manager.getBalance("set123", currencyId);
        REQUIRE(balance.value() == 0);

        // 设置负数余额应抛出异常
        REQUIRE_THROWS(manager.setBalance("set123", currencyId, -500, "测试负余额"));
    }

    SECTION("增加金钱") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("add123", "addplayer");

        manager.initializeNewPlayer("add123", "addplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("add123", currencyId, 1000);

        // 增加正数金额
        REQUIRE(manager.addMoney("add123", currencyId, 500, "测试增加"));
        auto balance = manager.getBalance("add123", currencyId);
        REQUIRE(balance.value() == 1500);

        // 增加零金额
        REQUIRE(manager.addMoney("add123", currencyId, 0, "增加零"));
        balance = manager.getBalance("add123", currencyId);
        REQUIRE(balance.value() == 1500);

        // 增加负数金额应抛出异常
        REQUIRE_THROWS(manager.addMoney("add123", currencyId, -200, "增加负数"));
        balance = manager.getBalance("add123", currencyId);
        REQUIRE(balance.value() == 1500);
    }

    SECTION("扣除金钱") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("red123", "redplayer");

        manager.initializeNewPlayer("red123", "redplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("red123", currencyId, 1000);

        // 扣除正数金额
        REQUIRE(manager.reduceMoney("red123", currencyId, 300, "测试扣除"));
        auto balance = manager.getBalance("red123", currencyId);
        REQUIRE(balance.value() == 700);

        // 扣除零金额
        REQUIRE(manager.reduceMoney("red123", currencyId, 0, "扣除零"));
        balance = manager.getBalance("red123", currencyId);
        REQUIRE(balance.value() == 700);

        // 扣除负数金额应抛出异常
        REQUIRE_THROWS(manager.reduceMoney("red123", currencyId, -100, "扣除负数"));
        balance = manager.getBalance("red123", currencyId);
        REQUIRE(balance.value() == 700);
    }

    SECTION("增加金钱超过最大余额限制") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("max123", "maxplayer");

        manager.initializeNewPlayer("max123", "maxplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        // 设置余额接近最大余额（1000000）
        manager.setBalance("max123", currencyId, 999500);

        // 增加金额超过最大余额应该失败
        REQUIRE_THROWS(manager.addMoney("max123", currencyId, 501, "超过最大余额"));

        // 余额不应该变化
        auto balance = manager.getBalance("max123", currencyId);
        REQUIRE(balance.value() == 999500);

        // 增加金额正好达到最大余额应该成功
        REQUIRE(manager.addMoney("max123", currencyId, 500, "达到最大余额"));
        balance = manager.getBalance("max123", currencyId);
        REQUIRE(balance.value() == 1000000);
    }
    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("EconomyManager 转账功能测试", "[economy][manager][transfer]") {
    auto cleanupGuard = SingletonCleanupGuard{};
    auto paths        = setupIsolatedManager(
        "economy_transfer",
        /*walMode*/ true,
        /*cacheSize*/ 2000,
        "NORMAL",
        /*initialBalance*/ 1000,
        /*maxBalance*/ 1000000,
        /*minTransferAmount*/ 1
    ); // 最小转账金额 > 0
    auto& manager = rlx_money::EconomyManager::getInstance();

    SECTION("基本转账功能") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("from123", "fromplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("to123", "toplayer");

        // 初始化玩家
        manager.initializeNewPlayer("from123", "fromplayer");
        manager.initializeNewPlayer("to123", "toplayer");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 设置初始余额
        manager.setBalance("from123", currencyId, 1000);
        manager.setBalance("to123", currencyId, 500);

        // 执行转账
        REQUIRE(manager.transferMoney("from123", "to123", currencyId, 300, "测试转账"));

        // 验证余额变化
        auto fromBalance = manager.getBalance("from123", currencyId);
        auto toBalance   = manager.getBalance("to123", currencyId);
        REQUIRE(fromBalance.value() == 700);
        REQUIRE(toBalance.value() == 800);
    }

    SECTION("转账到不存在的玩家") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("exist123", "existplayer");

        manager.initializeNewPlayer("exist123", "existplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("exist123", currencyId, 1000);

        // 转账到不存在的玩家应该失败
        REQUIRE_THROWS(manager.transferMoney("exist123", "nonexistent", currencyId, 100, "转账到不存在玩家"));

        // 余额不应该变化
        auto balance = manager.getBalance("exist123", currencyId);
        REQUIRE(balance.value() == 1000);
    }

    SECTION("从不存在的玩家转出") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("target123", "targetplayer");

        manager.initializeNewPlayer("target123", "targetplayer");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 从不存在的玩家转账应该失败
        REQUIRE_THROWS(manager.transferMoney("nonexistent", "target123", currencyId, 100, "从不存在玩家转账"));
    }

    SECTION("转账零金额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("zero123", "zeroplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("zero456", "zeroplayer2");

        manager.initializeNewPlayer("zero123", "zeroplayer");
        manager.initializeNewPlayer("zero456", "zeroplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("zero123", currencyId, 1000);
        manager.setBalance("zero456", currencyId, 500);

        // 转账零金额在最小转账金额>0下应失败，余额不变
        REQUIRE_THROWS(manager.transferMoney("zero123", "zero456", currencyId, 0, "转账零金额"));

        auto fromBalance = manager.getBalance("zero123", currencyId);
        auto toBalance   = manager.getBalance("zero456", currencyId);
        REQUIRE(fromBalance.value() == 1000);
        REQUIRE(toBalance.value() == 500);
    }

    SECTION("转账给自己") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("self123", "selfplayer");

        manager.initializeNewPlayer("self123", "selfplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("self123", currencyId, 1000);

        // 转账给自己应该失败
        REQUIRE_THROWS(manager.transferMoney("self123", "self123", currencyId, 100, "转账给自己"));

        // 余额不应该变化
        auto balance = manager.getBalance("self123", currencyId);
        REQUIRE(balance.value() == 1000);
    }

    SECTION("转账余额不足") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("poor123", "poorplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("rich123", "richplayer");

        manager.initializeNewPlayer("poor123", "poorplayer");
        manager.initializeNewPlayer("rich123", "richplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("poor123", currencyId, 500);
        manager.setBalance("rich123", currencyId, 2000);

        // 尝试转出超过余额的金额应该失败
        REQUIRE_THROWS(manager.transferMoney("poor123", "rich123", currencyId, 600, "余额不足转账"));

        // 余额不应该变化
        auto poorBalance = manager.getBalance("poor123", currencyId);
        auto richBalance = manager.getBalance("rich123", currencyId);
        REQUIRE(poorBalance.value() == 500);
        REQUIRE(richBalance.value() == 2000);
    }

    SECTION("转账导致超过最大余额") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("sender123", "senderplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("receiver123", "receiverplayer");

        manager.initializeNewPlayer("sender123", "senderplayer");
        manager.initializeNewPlayer("receiver123", "receiverplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        // 设置最大余额为1000000，接收方余额为999000
        manager.setBalance("receiver123", currencyId, 999000);
        manager.setBalance("sender123", currencyId, 5000);

        // 转账1001会导致接收方超过最大余额，应该失败
        REQUIRE_THROWS(manager.transferMoney("sender123", "receiver123", currencyId, 1001, "超过最大余额"));

        // 余额不应该变化
        auto receiverBalance = manager.getBalance("receiver123", currencyId);
        REQUIRE(receiverBalance.value() == 999000);

        // 转账正好达到最大余额应该成功
        REQUIRE(manager.transferMoney("sender123", "receiver123", currencyId, 1000, "达到最大余额"));
        receiverBalance = manager.getBalance("receiver123", currencyId);
        REQUIRE(receiverBalance.value() == 1000000);
    }

    SECTION("最小转账金额边界测试") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("min123", "minplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("min456", "minplayer2");

        manager.initializeNewPlayer("min123", "minplayer");
        manager.initializeNewPlayer("min456", "minplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("min123", currencyId, 1000);
        manager.setBalance("min456", currencyId, 500);

        // 最小转账金额为1，转账0应该失败
        REQUIRE_THROWS(manager.transferMoney("min123", "min456", currencyId, 0, "小于最小转账金额"));

        // 转账最小金额应该成功
        REQUIRE(manager.transferMoney("min123", "min456", currencyId, 1, "最小转账金额"));
        auto fromBalance = manager.getBalance("min123", currencyId);
        REQUIRE(fromBalance.value() == 999);
    }
    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("EconomyManager 转账手续费测试", "[economy][manager][transfer][fee]") {
    auto cleanupGuard = SingletonCleanupGuard{};

    SECTION("固定手续费测试") {
        auto paths = setupIsolatedManager(
            "economy_transfer_fee_fixed",
            /*walMode*/ true,
            /*cacheSize*/ 2000,
            "NORMAL",
            /*initialBalance*/ 1000,
            /*maxBalance*/ 1000000,
            /*minTransferAmount*/ 1,
            /*transferFee*/ 10, // 固定手续费10
            /*feePercentage*/ 0.0,
            /*allowPlayerTransfer*/ true
        );
        auto& manager = rlx_money::EconomyManager::getInstance();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("fee123", "feeplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("fee456", "feeplayer2");

        manager.initializeNewPlayer("fee123", "feeplayer");
        manager.initializeNewPlayer("fee456", "feeplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("fee123", currencyId, 1000);
        manager.setBalance("fee456", currencyId, 500);

        // 转账100，固定手续费10，转出方扣除110，转入方增加100
        REQUIRE(manager.transferMoney("fee123", "fee456", currencyId, 100, "固定手续费测试"));

        auto fromBalance = manager.getBalance("fee123", currencyId);
        auto toBalance   = manager.getBalance("fee456", currencyId);
        REQUIRE(fromBalance.value() == 890); // 1000 - 100 - 10
        REQUIRE(toBalance.value() == 600);   // 500 + 100

        cleanupFiles({paths.first, paths.second});
    }

    SECTION("百分比手续费测试") {
        auto paths = setupIsolatedManager(
            "economy_transfer_fee_percentage",
            /*walMode*/ true,
            /*cacheSize*/ 2000,
            "NORMAL",
            /*initialBalance*/ 1000,
            /*maxBalance*/ 1000000,
            /*minTransferAmount*/ 1,
            /*transferFee*/ 0,
            /*feePercentage*/ 5.0, // 5%手续费
            /*allowPlayerTransfer*/ true
        );
        auto& manager = rlx_money::EconomyManager::getInstance();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("pct123", "pctplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("pct456", "pctplayer2");

        manager.initializeNewPlayer("pct123", "pctplayer");
        manager.initializeNewPlayer("pct456", "pctplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("pct123", currencyId, 1000);
        manager.setBalance("pct456", currencyId, 500);

        // 转账100，5%手续费=5，转出方扣除105，转入方增加100
        REQUIRE(manager.transferMoney("pct123", "pct456", currencyId, 100, "百分比手续费测试"));

        auto fromBalance = manager.getBalance("pct123", currencyId);
        auto toBalance   = manager.getBalance("pct456", currencyId);
        REQUIRE(fromBalance.value() == 895); // 1000 - 100 - 5
        REQUIRE(toBalance.value() == 600);   // 500 + 100

        cleanupFiles({paths.first, paths.second});
    }

    SECTION("固定手续费和百分比手续费组合测试") {
        auto paths = setupIsolatedManager(
            "economy_transfer_fee_combined",
            /*walMode*/ true,
            /*cacheSize*/ 2000,
            "NORMAL",
            /*initialBalance*/ 1000,
            /*maxBalance*/ 1000000,
            /*minTransferAmount*/ 1,
            /*transferFee*/ 10,    // 固定手续费10
            /*feePercentage*/ 5.0, // 5%手续费
            /*allowPlayerTransfer*/ true
        );
        auto& manager = rlx_money::EconomyManager::getInstance();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("comb123", "combplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("comb456", "combplayer2");

        manager.initializeNewPlayer("comb123", "combplayer");
        manager.initializeNewPlayer("comb456", "combplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("comb123", currencyId, 1000);
        manager.setBalance("comb456", currencyId, 500);

        // 转账100，固定手续费10 + 5%手续费5 = 15，转出方扣除115，转入方增加100
        REQUIRE(manager.transferMoney("comb123", "comb456", currencyId, 100, "组合手续费测试"));

        auto fromBalance = manager.getBalance("comb123", currencyId);
        auto toBalance   = manager.getBalance("comb456", currencyId);
        REQUIRE(fromBalance.value() == 885); // 1000 - 100 - 15
        REQUIRE(toBalance.value() == 600);   // 500 + 100

        cleanupFiles({paths.first, paths.second});
    }

    SECTION("转账手续费导致余额不足") {
        auto paths = setupIsolatedManager(
            "economy_transfer_fee_insufficient",
            /*walMode*/ true,
            /*cacheSize*/ 2000,
            "NORMAL",
            /*initialBalance*/ 1000,
            /*maxBalance*/ 1000000,
            /*minTransferAmount*/ 1,
            /*transferFee*/ 10,
            /*feePercentage*/ 5.0,
            /*allowPlayerTransfer*/ true
        );
        auto& manager = rlx_money::EconomyManager::getInstance();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("insuf123", "insufplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("insuf456", "insufplayer2");

        manager.initializeNewPlayer("insuf123", "insufplayer");
        manager.initializeNewPlayer("insuf456", "insufplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("insuf123", currencyId, 100); // 余额100
        manager.setBalance("insuf456", currencyId, 500);

        // 转账100，但需要115（100+10+5），余额不足应该失败
        REQUIRE_THROWS(manager.transferMoney("insuf123", "insuf456", currencyId, 100, "手续费导致余额不足"));

        // 余额不应该变化
        auto fromBalance = manager.getBalance("insuf123", currencyId);
        REQUIRE(fromBalance.value() == 100);

        cleanupFiles({paths.first, paths.second});
    }
}

TEST_CASE("EconomyManager 转账功能禁用测试", "[economy][manager][transfer][disabled]") {
    auto cleanupGuard = SingletonCleanupGuard{};
    auto paths        = setupIsolatedManager(
        "economy_transfer_disabled",
        /*walMode*/ true,
        /*cacheSize*/ 2000,
        "NORMAL",
        /*initialBalance*/ 1000,
        /*maxBalance*/ 1000000,
        /*minTransferAmount*/ 1,
        /*transferFee*/ 0,
        /*feePercentage*/ 0.0,
        /*allowPlayerTransfer*/ false // 禁用转账功能
    );
    auto& manager = rlx_money::EconomyManager::getInstance();

    rlx_money::LeviLaminaAPI::clearMockPlayers();
    rlx_money::LeviLaminaAPI::addMockPlayer("disable123", "disableplayer");
    rlx_money::LeviLaminaAPI::addMockPlayer("disable456", "disableplayer2");

    manager.initializeNewPlayer("disable123", "disableplayer");
    manager.initializeNewPlayer("disable456", "disableplayer2");
    std::string currencyId = manager.getDefaultCurrencyId();
    manager.setBalance("disable123", currencyId, 1000);
    manager.setBalance("disable456", currencyId, 500);

    // 转账功能禁用时，转账应该失败
    REQUIRE_THROWS(manager.transferMoney("disable123", "disable456", currencyId, 100, "转账功能禁用"));

    // 余额不应该变化
    auto fromBalance = manager.getBalance("disable123", currencyId);
    auto toBalance   = manager.getBalance("disable456", currencyId);
    REQUIRE(fromBalance.value() == 1000);
    REQUIRE(toBalance.value() == 500);

    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("EconomyManager 手续费舍入与溢出保护测试", "[economy][manager][transfer][fee][rounding]") {
    auto cleanupGuard = SingletonCleanupGuard{};

    SECTION("2.5% 手续费下的四舍五入边界") {
        auto paths = setupIsolatedManager(
            "economy_transfer_fee_rounding_25",
            /*walMode*/ true,
            /*cacheSize*/ 2000,
            "NORMAL",
            /*initialBalance*/ 0,
            /*maxBalance*/ 1000000000, // 10亿，在int范围内
            /*minTransferAmount*/ 1,
            /*transferFee*/ 0,
            /*feePercentage*/ 2.5,
            /*allowPlayerTransfer*/ true
        );
        auto& manager = rlx_money::EconomyManager::getInstance();

        truncateAllTables();
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("r1_sender", "r1_sender");
        rlx_money::LeviLaminaAPI::addMockPlayer("r1_recv", "r1_recv");

        manager.initializeNewPlayer("r1_sender", "r1_sender");
        manager.initializeNewPlayer("r1_recv", "r1_recv");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("r1_sender", currencyId, 100000);
        manager.setBalance("r1_recv", currencyId, 0);

        // amount=20, 2.5% => 0.5 -> round=1, total=21
        REQUIRE(manager.transferMoney("r1_sender", "r1_recv", currencyId, 20, "round_20"));
        auto s1 = manager.getBalance("r1_sender", currencyId);
        auto r1 = manager.getBalance("r1_recv", currencyId);
        REQUIRE(s1.value() == 100000 - (20 + 1));
        REQUIRE(r1.value() == 0 + 20);

        // amount=40, 2.5% => 1.0 -> round=1, total=41
        REQUIRE(manager.transferMoney("r1_sender", "r1_recv", currencyId, 40, "round_40"));
        s1 = manager.getBalance("r1_sender", currencyId);
        r1 = manager.getBalance("r1_recv", currencyId);
        REQUIRE(s1.value() == 100000 - (20 + 1) - (40 + 1));
        REQUIRE(r1.value() == 20 + 40);

        // amount=60, 2.5% => 1.5 -> round=2, total=62
        REQUIRE(manager.transferMoney("r1_sender", "r1_recv", currencyId, 60, "round_60"));
        s1 = manager.getBalance("r1_sender", currencyId);
        r1 = manager.getBalance("r1_recv", currencyId);
        REQUIRE(s1.value() == 100000 - (21) - (41) - (62));
        REQUIRE(r1.value() == 20 + 40 + 60);

        cleanupFiles({paths.first, paths.second});
    }

    SECTION("大额转账下的余额充足性与溢出保护") {
        auto paths = setupIsolatedManager(
            "economy_transfer_fee_rounding_large",
            /*walMode*/ true,
            /*cacheSize*/ 2000,
            "NORMAL",
            /*initialBalance*/ 0,
            /*maxBalance*/ 2000000000, // 20亿，在int范围内
            /*minTransferAmount*/ 1,
            /*transferFee*/ 0,
            /*feePercentage*/ 1.0,
            /*allowPlayerTransfer*/ true
        );
        auto& manager = rlx_money::EconomyManager::getInstance();

        truncateAllTables();
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("lg_sender", "lg_sender");
        rlx_money::LeviLaminaAPI::addMockPlayer("lg_recv", "lg_recv");

        manager.initializeNewPlayer("lg_sender", "lg_sender");
        manager.initializeNewPlayer("lg_recv", "lg_recv");

        // 选择在 int 范围内的大数值
        const int amount = 1000000000; // 10亿
        // 1% 手续费 => 10,000,000
        const int expectedFee = 10000000;
        const int totalNeeded = amount + expectedFee; // 1,010,000,000

        std::string currencyId = manager.getDefaultCurrencyId();
        // 余额小于总额，应该抛余额不足
        manager.setBalance("lg_sender", currencyId, totalNeeded - 1);
        manager.setBalance("lg_recv", currencyId, 0);
        REQUIRE_THROWS(manager.transferMoney("lg_sender", "lg_recv", currencyId, amount, "large_insufficient"));

        // 余额刚好等于总额，应成功，且转入方增加 amount
        manager.setBalance("lg_sender", currencyId, totalNeeded);
        REQUIRE(manager.transferMoney("lg_sender", "lg_recv", currencyId, amount, "large_exact"));
        auto sb = manager.getBalance("lg_sender", currencyId);
        auto rb = manager.getBalance("lg_recv", currencyId);
        REQUIRE(sb.value() == 0);
        REQUIRE(rb.value() == amount);

        cleanupFiles({paths.first, paths.second});
    }
}

TEST_CASE("EconomyManager 工具函数测试", "[economy][manager][utils]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("economy_utils");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("金额验证") {
        // 测试有效金额
        REQUIRE(manager.isValidAmount(0));
        REQUIRE(manager.isValidAmount(100));
        REQUIRE_FALSE(manager.isValidAmount(-100));
        REQUIRE(manager.isValidAmount(INT_MAX));
        REQUIRE_FALSE(manager.isValidAmount(INT_MIN));
    }

    SECTION("余额充足性检查") {
        ensureDatabaseInitialized();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("check123", "checkplayer");

        manager.initializeNewPlayer("check123", "checkplayer");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("check123", currencyId, 1000);

        // 检查充足余额
        REQUIRE(manager.hasSufficientBalance("check123", currencyId, 500));
        REQUIRE(manager.hasSufficientBalance("check123", currencyId, 1000));
        REQUIRE_FALSE(manager.hasSufficientBalance("check123", currencyId, 1001));
        REQUIRE_FALSE(manager.hasSufficientBalance("check123", currencyId, 2000));

        // 检查不存在的玩家
        REQUIRE_FALSE(manager.hasSufficientBalance("nonexistent", currencyId, 100));
    }

    SECTION("服务器统计信息") {
        ensureDatabaseInitialized();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        truncateAllTables();
        rlx_money::LeviLaminaAPI::addMockPlayer("stat123", "statplayer");
        rlx_money::LeviLaminaAPI::addMockPlayer("stat456", "statplayer2");

        manager.initializeNewPlayer("stat123", "statplayer");
        manager.initializeNewPlayer("stat456", "statplayer2");
        std::string currencyId = manager.getDefaultCurrencyId();

        manager.setBalance("stat123", currencyId, 1000);
        manager.setBalance("stat456", currencyId, 2000);

        // 检查总财富
        REQUIRE(manager.getTotalWealth(currencyId) == 3000);

        // 检查玩家数量
        REQUIRE(manager.getPlayerCount() == 2);
    }
    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("EconomyManager 操作者信息测试", "[economy][manager][operator]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("economy_operator");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("带操作者信息的余额操作") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("op123", "opplayer");

        manager.initializeNewPlayer("op123", "opplayer");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 管理员操作
        REQUIRE(manager.setBalance("op123", currencyId, 1000, rlx_money::OperatorType::ADMIN, "admin_user"));

        // 商店操作
        REQUIRE(manager.addMoney("op123", currencyId, 200, rlx_money::OperatorType::SHOP, "test_shop"));

        // 地产商操作
        REQUIRE(manager.reduceMoney("op123", currencyId, 100, rlx_money::OperatorType::REAL_ESTATE, "estate_agent"));

        auto balance = manager.getBalance("op123", currencyId);
        REQUIRE(balance.value() == 1100); // 1000 + 200 - 100
    }
    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("EconomyManager 排行榜功能测试", "[economy][manager][leaderboard]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("economy_leaderboard");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("财富排行榜") {
        ensureDatabaseInitialized();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        truncateAllTables();

        // 创建多个测试玩家
        std::vector<std::pair<std::string, std::string>> players = {
            {"rich1", "rich_player1"},
            {"rich2", "rich_player2"},
            {"rich3", "rich_player3"},
            {"rich4", "rich_player4"},
            {"rich5", "rich_player5"}
        };

        // 初始化玩家并设置余额
        std::string      currencyId = manager.getDefaultCurrencyId();
        std::vector<int> balances   = {5000, 3000, 8000, 1000, 6000};
        for (size_t i = 0; i < players.size(); ++i) {
            rlx_money::LeviLaminaAPI::addMockPlayer(players[i].first, players[i].second);
            manager.initializeNewPlayer(players[i].first, players[i].second);
            manager.setBalance(players[i].first, currencyId, balances[i]);
        }

        // 获取排行榜
        auto leaderboard = manager.getTopBalanceList(currencyId, 3);

        // 验证排行榜长度
        REQUIRE(leaderboard.size() == 3);

        // 验证排行榜顺序（应该是降序）
        REQUIRE(leaderboard[0].balance == 8000); // rich3
        REQUIRE(leaderboard[1].balance == 6000); // rich5
        REQUIRE(leaderboard[2].balance == 5000); // rich1

        // 验证排名
        REQUIRE(leaderboard[0].rank == 1);
        REQUIRE(leaderboard[1].rank == 2);
        REQUIRE(leaderboard[2].rank == 3);
    }

    SECTION("空排行榜") {
        ensureDatabaseInitialized();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        truncateAllTables();

        // 没有玩家时的排行榜
        std::string currencyId       = manager.getDefaultCurrencyId();
        auto        emptyLeaderboard = manager.getTopBalanceList(currencyId, 5);
        REQUIRE(emptyLeaderboard.empty());
    }

    SECTION("超出玩家数量的排行榜请求") {
        ensureDatabaseInitialized();

        rlx_money::LeviLaminaAPI::clearMockPlayers();
        truncateAllTables();
        rlx_money::LeviLaminaAPI::addMockPlayer("single", "single_player");

        manager.initializeNewPlayer("single", "single_player");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("single", currencyId, 1000);

        // 请求超过玩家数量的排行榜
        auto leaderboard = manager.getTopBalanceList(currencyId, 10);
        REQUIRE(leaderboard.size() == 1);
        REQUIRE(leaderboard[0].balance == 1000);
        REQUIRE(leaderboard[0].rank == 1);
    }
    cleanupFiles({paths.first, paths.second});
}

TEST_CASE("EconomyManager 交易历史测试", "[economy][manager][history]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("economy_history");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("玩家交易记录") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("history123", "history_player");

        manager.initializeNewPlayer("history123", "history_player");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 执行一系列操作
        manager.setBalance("history123", currencyId, 1000, "初始设置");
        manager.addMoney("history123", currencyId, 500, "奖励");
        manager.reduceMoney("history123", currencyId, 200, "消费");
        manager.addMoney("history123", currencyId, 300, "活动奖励");

        // 获取交易记录
        auto transactions = manager.getPlayerTransactions("history123", currencyId, 1, 10);

        // 验证交易记录数量
        REQUIRE(transactions.size() >= 4); // 至少包含4条交易记录

        // 获取交易总数
        int totalCount = manager.getPlayerTransactionCount("history123");
        REQUIRE(totalCount >= 4);

        // 不存在玩家的交易记录
        auto emptyTransactions = manager.getPlayerTransactions("nonexistent", currencyId, 1, 10);
        REQUIRE(emptyTransactions.empty());

        int emptyCount = manager.getPlayerTransactionCount("nonexistent");
        REQUIRE(emptyCount == 0);
    }

    SECTION("交易记录分页") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("page123", "page_player");

        manager.initializeNewPlayer("page123", "page_player");
        std::string currencyId = manager.getDefaultCurrencyId();

        // 创建多个交易记录
        for (int i = 0; i < 25; ++i) {
            manager.addMoney("page123", currencyId, 10, "交易 " + std::to_string(i + 1));
        }

        // 测试分页
        auto page1 = manager.getPlayerTransactions("page123", currencyId, 1, 10);
        REQUIRE(page1.size() == 10);

        auto page2 = manager.getPlayerTransactions("page123", currencyId, 2, 10);
        REQUIRE(page2.size() == 10);

        auto page3 = manager.getPlayerTransactions("page123", currencyId, 3, 10);
        // 注意：initializeNewPlayer 会生成一条 INITIAL 记录，因此总数为 25 + 1 = 26，第三页应为 6 条
        REQUIRE(page3.size() == 6);

        // 超出范围的页面
        auto emptyPage = manager.getPlayerTransactions("page123", currencyId, 4, 10);
        REQUIRE(emptyPage.empty());
    }
    cleanupFiles({paths.first, paths.second});
}

// ============================================================================
// EconomyManager 单线程稳定性测试
// ============================================================================

TEST_CASE("EconomyManager 单线程稳定性测试", "[economy][manager][stability]") {
    auto  cleanupGuard = SingletonCleanupGuard{};
    auto  paths        = setupIsolatedManager("economy_stability");
    auto& manager      = rlx_money::EconomyManager::getInstance();

    SECTION("大量连续操作稳定性") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("stable123", "stable_player");

        manager.initializeNewPlayer("stable123", "stable_player");
        std::string currencyId = manager.getDefaultCurrencyId();
        manager.setBalance("stable123", currencyId, 1000);

        const int numOperations   = 1000;
        int       expectedBalance = 1000;

        // 执行大量连续操作
        for (int i = 0; i < numOperations; ++i) {
            if (i % 3 == 0) {
                // 增加操作
                manager.addMoney("stable123", currencyId, 2, "稳定性测试增加 " + std::to_string(i));
                expectedBalance += 2;
            } else if (i % 3 == 1) {
                // 扣除操作
                manager.reduceMoney("stable123", currencyId, 1, "稳定性测试扣除 " + std::to_string(i));
                expectedBalance -= 1;
            } else {
                // 设置操作
                expectedBalance = i * 5;
                manager.setBalance("stable123", currencyId, expectedBalance, "稳定性测试设置 " + std::to_string(i));
            }
        }

        // 验证最终余额
        auto finalBalance = manager.getBalance("stable123", currencyId);
        REQUIRE(finalBalance.has_value());
        REQUIRE(finalBalance.value() == expectedBalance);
    }

    SECTION("连续转账稳定性") {
        rlx_money::LeviLaminaAPI::clearMockPlayers();
        rlx_money::LeviLaminaAPI::addMockPlayer("user1", "user1");
        rlx_money::LeviLaminaAPI::addMockPlayer("user2", "user2");
        rlx_money::LeviLaminaAPI::addMockPlayer("user3", "user3");

        manager.initializeNewPlayer("user1", "user1");
        manager.initializeNewPlayer("user2", "user2");
        manager.initializeNewPlayer("user3", "user3");
        std::string currencyId = manager.getDefaultCurrencyId();

        manager.setBalance("user1", currencyId, 3000);
        manager.setBalance("user2", currencyId, 2000);
        manager.setBalance("user3", currencyId, 1000);

        const int numTransfers = 500;

        // 执行大量连续转账
        for (int i = 0; i < numTransfers; ++i) {
            if (i % 3 == 0) {
                manager.transferMoney("user1", "user2", currencyId, 5, "连续转账 " + std::to_string(i));
            } else if (i % 3 == 1) {
                manager.transferMoney("user2", "user3", currencyId, 3, "连续转账 " + std::to_string(i));
            } else {
                manager.transferMoney("user3", "user1", currencyId, 2, "连续转账 " + std::to_string(i));
            }
        }

        // 验证最终余额总和应该保持不变（扣除转账手续费的话会减少，这里手续费为0）
        auto balance1 = manager.getBalance("user1", currencyId);
        auto balance2 = manager.getBalance("user2", currencyId);
        auto balance3 = manager.getBalance("user3", currencyId);

        REQUIRE(balance1.has_value());
        REQUIRE(balance2.has_value());
        REQUIRE(balance3.has_value());

        // 初始总余额: 3000 + 2000 + 1000 = 6000
        // 转账过程：user1转出(500/3)*5≈833, user2转出(500/3)*3≈500, user3转出(500/3)*2≈333
        // user2从user1转入833, user3从user2转入500, user1从user3转入333
        // 由于转账是循环的，总余额应该保持6000
        REQUIRE(balance1.value() + balance2.value() + balance3.value() == 6000);
    }
    cleanupFiles({paths.first, paths.second});
}

// ============================================================================
// 类型转换工具函数测试
// ============================================================================

TEST_CASE("经济类型转换工具函数测试", "[economy][utils][types][conversion]") {
    auto cleanupGuard = SingletonCleanupGuard{};

    SECTION("TransactionType 转换测试") {
        // 测试 transactionTypeToString - 修正为实际的小写输出
        REQUIRE(rlx_money::transactionTypeToString(rlx_money::TransactionType::SET) == "set");
        REQUIRE(rlx_money::transactionTypeToString(rlx_money::TransactionType::ADD) == "add");
        REQUIRE(rlx_money::transactionTypeToString(rlx_money::TransactionType::REDUCE) == "reduce");
        REQUIRE(rlx_money::transactionTypeToString(rlx_money::TransactionType::TRANSFER) == "transfer");
        REQUIRE(rlx_money::transactionTypeToString(rlx_money::TransactionType::INITIAL) == "initial");

        // 测试 stringToTransactionType - 修正为实际接受的小写输入
        REQUIRE(rlx_money::stringToTransactionType("set") == rlx_money::TransactionType::SET);
        REQUIRE(rlx_money::stringToTransactionType("add") == rlx_money::TransactionType::ADD);
        REQUIRE(rlx_money::stringToTransactionType("reduce") == rlx_money::TransactionType::REDUCE);
        REQUIRE(rlx_money::stringToTransactionType("transfer") == rlx_money::TransactionType::TRANSFER);
        REQUIRE(rlx_money::stringToTransactionType("initial") == rlx_money::TransactionType::INITIAL);

        // 测试大小写敏感（实际实现是大小写敏感的）
        REQUIRE_THROWS(rlx_money::stringToTransactionType("SET"));
        REQUIRE_THROWS(rlx_money::stringToTransactionType("Add"));

        // 测试无效字符串（可能抛出异常或返回默认值）
        REQUIRE_THROWS(rlx_money::stringToTransactionType("INVALID"));
        REQUIRE_THROWS(rlx_money::stringToTransactionType(""));
        REQUIRE_THROWS(rlx_money::stringToTransactionType("unknown"));
    }

    SECTION("OperatorType 转换测试") {
        // 测试 operatorTypeToString - 修正为实际的中文输出
        REQUIRE(rlx_money::operatorTypeToString(rlx_money::OperatorType::ADMIN) == "管理员");
        REQUIRE(rlx_money::operatorTypeToString(rlx_money::OperatorType::SHOP) == "商店");
        REQUIRE(rlx_money::operatorTypeToString(rlx_money::OperatorType::REAL_ESTATE) == "地产商");
        REQUIRE(rlx_money::operatorTypeToString(rlx_money::OperatorType::SYSTEM) == "系统");
        REQUIRE(rlx_money::operatorTypeToString(rlx_money::OperatorType::PLAYER) == "玩家");
        REQUIRE(rlx_money::operatorTypeToString(rlx_money::OperatorType::OTHER) == "其他");
    }

    SECTION("ErrorCode 转换测试") {
        // 测试 errorCodeToString - 修正为实际的中文输出
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::SUCCESS) == "成功");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::PLAYER_NOT_FOUND) == "玩家不存在");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::INSUFFICIENT_BALANCE) == "余额不足");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::INVALID_AMOUNT) == "无效金额");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::DATABASE_ERROR) == "数据库错误");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::PERMISSION_DENIED) == "权限不足");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::TRANSFER_DISABLED) == "转账功能已禁用");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::CONFIG_ERROR) == "配置错误");
        REQUIRE(rlx_money::errorCodeToString(rlx_money::ErrorCode::PLAYER_ALREADY_EXISTS) == "玩家已存在");
    }

    SECTION("交易描述生成测试") {
        // 测试基本描述生成 - 修正为实际的中文输出
        REQUIRE(
            rlx_money::describe(rlx_money::TransactionType::SET, 1000, rlx_money::MoneyFlow::NEUTRAL)
            == "管理员设置余额为 1000"
        );
        REQUIRE(
            rlx_money::describe(rlx_money::TransactionType::ADD, 500, rlx_money::MoneyFlow::CREDIT) == "获得 500 金币"
        );
        REQUIRE(
            rlx_money::describe(rlx_money::TransactionType::REDUCE, 200, rlx_money::MoneyFlow::DEBIT) == "消费 200 金币"
        );
        REQUIRE(
            rlx_money::describe(rlx_money::TransactionType::TRANSFER, 300, rlx_money::MoneyFlow::DEBIT, "target_player")
            == "向 target_player 转账 300 金币"
        );
        REQUIRE(
            rlx_money::describe(rlx_money::TransactionType::INITIAL, 0, rlx_money::MoneyFlow::NEUTRAL)
            == "新玩家初始金额 0"
        );

        // 测试带操作者信息的描述生成
        REQUIRE(
            rlx_money::describe(
                rlx_money::TransactionType::ADD,
                500,
                rlx_money::MoneyFlow::CREDIT,
                rlx_money::OperatorType::ADMIN,
                "admin_user"
            )
            == "从管理员[admin_user]获得 500 金币"
        );

        REQUIRE(
            rlx_money::describe(
                rlx_money::TransactionType::REDUCE,
                200,
                rlx_money::MoneyFlow::DEBIT,
                rlx_money::OperatorType::SHOP,
                "test_shop"
            )
            == "向商店[test_shop]消费 200 金币"
        );

        REQUIRE(
            rlx_money::describe(
                rlx_money::TransactionType::TRANSFER,
                300,
                rlx_money::MoneyFlow::DEBIT,
                rlx_money::OperatorType::PLAYER,
                "player1",
                "player2"
            )
            == "向 player2 转账 300 金币"
        );
    }
}

// ============================================================================
// 数据结构测试
// ============================================================================

TEST_CASE("经济数据结构测试", "[economy][data][structures]") {
    auto cleanupGuard = SingletonCleanupGuard{};

    SECTION("PlayerData 结构测试") {
        // 默认构造函数
        rlx_money::PlayerData player1;
        REQUIRE(player1.xuid.empty());
        REQUIRE(player1.username.empty());
        REQUIRE(player1.firstJoinTime == 0);
        REQUIRE(player1.createdAt == 0);
        REQUIRE(player1.updatedAt == 0);

        // 带参数构造函数（不再包含balance字段）
        rlx_money::PlayerData player2("12345", "testplayer", 1600000000);
        REQUIRE(player2.xuid == "12345");
        REQUIRE(player2.username == "testplayer");
        REQUIRE(player2.firstJoinTime == 1600000000);
        REQUIRE(player2.createdAt == 1600000000);
        REQUIRE(player2.updatedAt == 1600000000);
    }

    SECTION("TransactionRecord 结构测试") {
        // 默认构造函数
        rlx_money::TransactionRecord record1;
        REQUIRE(record1.id == 0);
        REQUIRE(record1.xuid.empty());
        REQUIRE(record1.currencyId.empty());
        REQUIRE(record1.amount == 0);
        REQUIRE(record1.balance == 0);
        REQUIRE(record1.type == rlx_money::TransactionType::SET);
        REQUIRE(record1.description.empty());
        REQUIRE(record1.timestamp == 0);
        REQUIRE_FALSE(record1.relatedXuid.has_value());

        // 带参数构造函数（包含currencyId）
        rlx_money::TransactionRecord
            record2(1, "12345", "gold", 500, 1500, rlx_money::TransactionType::ADD, "测试交易", 1600000000, "67890");
        REQUIRE(record2.id == 1);
        REQUIRE(record2.xuid == "12345");
        REQUIRE(record2.currencyId == "gold");
        REQUIRE(record2.amount == 500);
        REQUIRE(record2.balance == 1500);
        REQUIRE(record2.type == rlx_money::TransactionType::ADD);
        REQUIRE(record2.description == "测试交易");
        REQUIRE(record2.timestamp == 1600000000);
        REQUIRE(record2.relatedXuid.has_value());
        REQUIRE(record2.relatedXuid.value() == "67890");
    }

    SECTION("PlayerBalance 结构测试") {
        // 默认构造函数
        rlx_money::PlayerBalance balance1;
        REQUIRE(balance1.xuid.empty());
        REQUIRE(balance1.currencyId.empty());
        REQUIRE(balance1.balance == 0);
        REQUIRE(balance1.updatedAt == 0);

        // 带参数构造函数
        rlx_money::PlayerBalance balance2("12345", "gold", 1000);
        REQUIRE(balance2.xuid == "12345");
        REQUIRE(balance2.currencyId == "gold");
        REQUIRE(balance2.balance == 1000);
    }

    SECTION("TopBalanceEntry 结构测试") {
        // 默认构造函数
        rlx_money::TopBalanceEntry entry1;
        REQUIRE(entry1.username.empty());
        REQUIRE(entry1.xuid.empty());
        REQUIRE(entry1.currencyId.empty());
        REQUIRE(entry1.balance == 0);
        REQUIRE(entry1.rank == 0);

        // 带参数构造函数（包含currencyId）
        rlx_money::TopBalanceEntry entry2("richplayer", "98765", "gold", 10000, 1);
        REQUIRE(entry2.username == "richplayer");
        REQUIRE(entry2.xuid == "98765");
        REQUIRE(entry2.currencyId == "gold");
        REQUIRE(entry2.balance == 10000);
        REQUIRE(entry2.rank == 1);
    }
}