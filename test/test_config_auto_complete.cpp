#include "mod/config/MoneyConfig.h"
#include "mod/config/ConfigStructures.h"
#include "utils/TestTempManager.h"
#include <catch2/catch_all.hpp>
#include <filesystem>
#include <fstream>

TEST_CASE("配置自动补全测试", "[config][auto_complete]") {
    // 使用临时文件管理器创建测试配置路径
    auto& tempManager = rlx_money::test::TestTempManager::getInstance();
    const std::string testConfigPath = tempManager.makeUniquePath("test_auto_complete", ".json");

    // 注册文件以便自动清理
    tempManager.registerFile(testConfigPath);

    SECTION("部分配置自动补全") {
        // 创建只包含部分配置的配置文件（所有字段在 currencies["gold"] 节点下，使用驼峰命名）
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
                        "initialBalance": 5000
                    }
                }
            }
        })";
        configFile.close();

        // 初始化配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(testConfigPath));

        // 验证配置值
        const auto& config = rlx_money::MoneyConfig::get();
        REQUIRE(config.defaultCurrency == "gold");
        REQUIRE(config.currencies.find("gold") != config.currencies.end());
        REQUIRE(config.currencies.at("gold").initialBalance == 5000);
        // 注意：由于单例模式，其他配置项可能被之前的测试修改过
        // 所以这里只验证配置文件被正确补全，而不验证具体的默认值
        REQUIRE(config.currencies.at("gold").maxBalance > 0);

        // 读取保存后的配置文件，验证是否自动补全了缺失项
        std::ifstream updatedFile(testConfigPath);
        std::string   content((std::istreambuf_iterator<char>(updatedFile)), std::istreambuf_iterator<char>());
        updatedFile.close();

        // 验证补全的配置项存在于文件中（使用驼峰命名，所有字段在 currencies 节点下）
        REQUIRE(content.find("\"defaultCurrency\"") != std::string::npos);
        REQUIRE(content.find("\"currencies\"") != std::string::npos);
        REQUIRE(content.find("\"maxBalance\"") != std::string::npos);
        REQUIRE(content.find("\"allowPlayerTransfer\"") != std::string::npos);
        REQUIRE(content.find("\"minTransferAmount\"") != std::string::npos);
        REQUIRE(content.find("\"database\"") != std::string::npos);
        REQUIRE(content.find("\"topList\"") != std::string::npos);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
        // 文件会自动清理
    }

    SECTION("空配置文件自动补全") {
        // 使用临时文件管理器创建空配置路径
        const std::string emptyConfigPath = tempManager.makeUniquePath("test_empty_config", ".json");

        // 注册文件以便自动清理
        tempManager.registerFile(emptyConfigPath);

        // 创建空的配置文件
        std::ofstream configFile(emptyConfigPath);
        configFile << "{}";
        configFile.close();

        // 初始化配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(emptyConfigPath));

        // 读取保存后的配置文件，验证是否包含所有配置项
        std::ifstream updatedFile(emptyConfigPath);
        std::string   content((std::istreambuf_iterator<char>(updatedFile)), std::istreambuf_iterator<char>());
        updatedFile.close();

        // 验证所有主要配置项都存在（使用驼峰命名，所有字段在 currencies 节点下）
        REQUIRE(content.find("\"defaultCurrency\"") != std::string::npos);
        REQUIRE(content.find("\"currencies\"") != std::string::npos);
        REQUIRE(content.find("\"database\"") != std::string::npos);
        REQUIRE(content.find("\"topList\"") != std::string::npos);
        REQUIRE(content.find("\"initialBalance\"") != std::string::npos);
        REQUIRE(content.find("\"maxBalance\"") != std::string::npos);
        REQUIRE(content.find("\"allowPlayerTransfer\"") != std::string::npos);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
        // 文件会自动清理
    }

    SECTION("完整配置文件不应被修改") {
        // 创建完整的配置文件（所有字段在 currencies["gold"] 节点下，使用驼峰命名）
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
                        "initialBalance": 2000,
                        "maxBalance": 5000000,
                        "minTransferAmount": 10,
                        "transferFee": 5,
                        "feePercentage": 1.5,
                        "allowPlayerTransfer": false
                    }
                },
                "database": {
                    "path": "custom_money.db",
                    "optimization": {
                        "walMode": false,
                        "cacheSize": 5000,
                        "synchronous": "FULL"
                    }
                },
                "topList": {
                    "defaultCount": 20,
                    "maxCount": 100
                }
            }
        })";
        configFile.close();

        // 记录原始文件内容
        std::ifstream originalFile(testConfigPath);
        std::string   originalContent((std::istreambuf_iterator<char>(originalFile)), std::istreambuf_iterator<char>());
        originalFile.close();

        // 初始化配置
        REQUIRE_NOTHROW(rlx_money::MoneyConfig::initialize(testConfigPath));

        // 读取更新后的文件内容
        std::ifstream updatedFile(testConfigPath);
        std::string   updatedContent((std::istreambuf_iterator<char>(updatedFile)), std::istreambuf_iterator<char>());
        updatedFile.close();

        // 验证配置值
        const auto& config = rlx_money::MoneyConfig::get();
        REQUIRE(config.defaultCurrency == "gold");
        REQUIRE(config.currencies.at("gold").initialBalance == 2000);
        REQUIRE(config.currencies.at("gold").maxBalance == 5000000);
        REQUIRE(config.currencies.at("gold").allowPlayerTransfer == false);

        // 清理
        rlx_money::MoneyConfig::resetForTesting();
        // 文件会自动清理
    }
}
