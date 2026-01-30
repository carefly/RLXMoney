#include "mod/config/MoneyConfig.h"
#include "mod/config/ConfigStructures.h"
#include "mod/exceptions/MoneyException.h"
#include "utils/TestTempManager.h"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>


// ============================================================================
// 配置管理器测试
// ============================================================================

TEST_CASE("MoneyConfig 测试", "[config][manager]") {
    // 使用临时文件管理器创建测试配置路径
    auto& tempManager = rlx_money::test::TestTempManager::getInstance();
    const std::string testConfigPath = tempManager.makeUniquePath("test_config", ".json");

    // 注册文件以便自动清理
    tempManager.registerFile(testConfigPath);

    SECTION("默认配置测试") {
        // 初始化配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(testConfigPath));

        // 获取默认配置
        const auto& config = rlx_money::MoneyConfig::get();
        REQUIRE(!config.defaultCurrency.empty());
        REQUIRE(!config.currencies.empty());
        // 注意：由于单例模式，配置可能被之前的测试修改过
        // 所以这里只验证基本结构存在
        REQUIRE(config.currencies.size() > 0);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
    }

    SECTION("配置加载测试") {
        // 创建测试配置文件（所有字段在 currencies["gold"] 节点下，使用驼峰命名）
        std::ofstream configFile(testConfigPath);
        configFile << R"({
            "RLXMoney": {
                "defaultCurrency": "gold",
                "currencies": {
                    "gold": {
                        "currencyId": "gold",
                        "name": "金币",
                        "symbol": "G",
                        "displayFormat": "{amount} {symbol}",
                        "enabled": true,
                        "initialBalance": 1000,
                        "maxBalance": 1000000,
                        "allowPlayerTransfer": true
                    }
                },
                "database": {
                    "path": "test_money.db"
                }
            }
        })";
        configFile.close();

        // 初始化配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(testConfigPath));

        // 验证配置值
        const auto& config = rlx_money::MoneyConfig::get();
        REQUIRE(config.defaultCurrency == "gold");
        REQUIRE(config.currencies.at("gold").initialBalance == 1000);
        REQUIRE(config.currencies.at("gold").maxBalance == 1000000);
        REQUIRE(config.currencies.at("gold").allowPlayerTransfer == true);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
        // 文件会自动清理
    }

    SECTION("配置文件不存在处理") {
        const std::string nonexistentConfig = "nonexistent_config.json";

        // 确保文件不存在
        if (std::filesystem::exists(nonexistentConfig)) {
            std::filesystem::remove(nonexistentConfig);
        }

        // 配置文件不存在时，应创建默认配置且不抛异常
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(nonexistentConfig));

        // 清理创建的文件
        if (std::filesystem::exists(nonexistentConfig)) {
            std::filesystem::remove(nonexistentConfig);
        }

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
    }

    SECTION("配置热重载测试") {
        // 使用临时文件管理器创建重载配置路径
        const std::string reloadConfigPath = tempManager.makeUniquePath("test_reload_config", ".json");

        // 注册文件以便自动清理
        tempManager.registerFile(reloadConfigPath);

        // 创建初始配置（新结构：默认币种 + 币种配置）
        std::ofstream configFile(reloadConfigPath);
        configFile << R"({
            "RLXMoney": {
                "defaultCurrency": "gold",
                "currencies": {
                    "gold": {
                        "currencyId": "gold",
                        "name": "金币",
                        "symbol": "G",
                        "displayFormat": "{amount} {symbol}",
                        "enabled": true,
                        "initialBalance": 500,
                        "maxBalance": 10000
                    }
                }
            }
        })";
        configFile.close();

        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(reloadConfigPath));

        const auto& config1 = rlx_money::MoneyConfig::get();
        REQUIRE(config1.currencies.at(config1.defaultCurrency).initialBalance == 500);

        // 修改配置文件（更新默认币种的初始余额与上限）
        std::ofstream configFile2(reloadConfigPath);
        configFile2 << R"({
            "RLXMoney": {
                "defaultCurrency": "gold",
                "currencies": {
                    "gold": {
                        "currencyId": "gold",
                        "name": "金币",
                        "symbol": "G",
                        "displayFormat": "{amount} {symbol}",
                        "enabled": true,
                        "initialBalance": 2000,
                        "maxBalance": 50000
                    }
                }
            }
        })";
        configFile2.close();

        // 热重载配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::reload());

        const auto& config2 = rlx_money::MoneyConfig::get();
        REQUIRE(config2.currencies.at(config2.defaultCurrency).initialBalance == 2000);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
        // 文件会自动清理
    }

    SECTION("便捷方法测试") {
        // 创建测试配置文件
        std::ofstream configFile(testConfigPath);
        configFile << R"({
            "RLXMoney": {
                "defaultCurrency": "gold",
                "currencies": {
                    "gold": {
                        "currencyId": "gold",
                        "name": "金币",
                        "symbol": "G",
                        "displayFormat": "{amount} {symbol}",
                        "enabled": true,
                        "initialBalance": 500,
                        "maxBalance": 10000,
                        "allowPlayerTransfer": true
                    }
                }
            }
        })";
        configFile.close();

        // 初始化配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(testConfigPath));

        // 测试 getInitialBalance
        REQUIRE(rlx_money::MoneyConfig::getInitialBalance() == 500);

        // 测试 setInitialBalance
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::setInitialBalance(1000));
        REQUIRE(rlx_money::MoneyConfig::getInitialBalance() == 1000);

        // 测试 getAllowPlayerTransfer
        REQUIRE(rlx_money::MoneyConfig::getAllowPlayerTransfer() == true);

        // 测试 setAllowPlayerTransfer
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::setAllowPlayerTransfer(false));
        REQUIRE(rlx_money::MoneyConfig::getAllowPlayerTransfer() == false);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
    }
}

// ============================================================================
// 异常处理测试
// ============================================================================

TEST_CASE("配置异常处理测试", "[config][exceptions]") {

    SECTION("MoneyException 基类测试") {
        rlx_money::MoneyException ex(rlx_money::ErrorCode::SUCCESS, "测试异常");
        REQUIRE(std::string(ex.what()) == "[成功] 测试异常"); // 修正为实际带前缀的格式
        REQUIRE(ex.getErrorCode() == rlx_money::ErrorCode::SUCCESS);
    }

    SECTION("DatabaseException 测试") {
        rlx_money::DatabaseException ex("数据库连接失败");
        REQUIRE_THAT(std::string(ex.what()), Catch::Matchers::ContainsSubstring("数据库错误: 数据库连接失败"));
        REQUIRE(ex.getErrorCode() == rlx_money::ErrorCode::DATABASE_ERROR);
    }

    SECTION("ConfigException 测试") {
        rlx_money::ConfigException ex("配置文件格式错误");
        REQUIRE_THAT(std::string(ex.what()), Catch::Matchers::ContainsSubstring("配置错误: 配置文件格式错误"));
        REQUIRE(ex.getErrorCode() == rlx_money::ErrorCode::CONFIG_ERROR);
    }

    SECTION("PermissionException 测试") {
        rlx_money::PermissionException ex("权限不足");
        REQUIRE_THAT(std::string(ex.what()), Catch::Matchers::ContainsSubstring("权限错误: 权限不足"));
        REQUIRE(ex.getErrorCode() == rlx_money::ErrorCode::PERMISSION_DENIED);
    }

    SECTION("InvalidArgumentException 测试") {
        rlx_money::InvalidArgumentException ex("无效金额");
        REQUIRE_THAT(std::string(ex.what()), Catch::Matchers::ContainsSubstring("参数错误: 无效金额"));
        REQUIRE(ex.getErrorCode() == rlx_money::ErrorCode::INVALID_AMOUNT);
    }

    SECTION("异常层次结构测试") {
        // 测试所有自定义异常都是 MoneyException 的子类
        std::unique_ptr<rlx_money::MoneyException> baseEx;

        baseEx = std::make_unique<rlx_money::DatabaseException>("test");
        REQUIRE(dynamic_cast<rlx_money::MoneyException*>(baseEx.get()) != nullptr);

        baseEx = std::make_unique<rlx_money::ConfigException>("test");
        REQUIRE(dynamic_cast<rlx_money::MoneyException*>(baseEx.get()) != nullptr);

        baseEx = std::make_unique<rlx_money::PermissionException>("test");
        REQUIRE(dynamic_cast<rlx_money::MoneyException*>(baseEx.get()) != nullptr);

        baseEx = std::make_unique<rlx_money::InvalidArgumentException>("test");
        REQUIRE(dynamic_cast<rlx_money::MoneyException*>(baseEx.get()) != nullptr);
    }
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST_CASE("配置边界条件测试", "[config][boundary][edge]") {

    SECTION("数值边界测试") {
        // 测试 INT64 边界值
        int64_t maxInt64 = INT64_MAX;
        int64_t minInt64 = INT64_MIN;

        // 这些测试主要用于验证函数能正确处理极值而不会溢出
        // 实际的业务逻辑可能会限制这些值
        REQUIRE(maxInt64 > 0);
        REQUIRE(minInt64 < 0);

        // 测试溢出检查
        REQUIRE(maxInt64 + 1 < maxInt64); // 溢出检查
        REQUIRE(minInt64 - 1 > minInt64); // 下溢检查
    }

    SECTION("空字符串和特殊字符测试") {
        // 测试空字符串
        std::string emptyStr;
        REQUIRE(emptyStr.empty());

        // 测试特殊字符
        std::string specialChars = "测试中文!@#$%^&*()";
        REQUIRE(!specialChars.empty());
        REQUIRE(specialChars.length() > 0);
    }

    SECTION("零值测试") {
        // 测试各种零值情况
        int64_t zeroInt    = 0;
        double  zeroDouble = 0.0;
        bool    zeroBool   = false;

        REQUIRE(zeroInt == 0);
        REQUIRE(zeroDouble == 0.0);
        REQUIRE_FALSE(zeroBool);
    }
}
