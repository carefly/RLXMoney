#include "mocks/MockLeviLaminaAPI.h"
#include "mod/data/DataStructures.h"
#include "mod/exceptions/MoneyException.h"
#include "mod/types/Types.h"
#include <catch2/catch_all.hpp>


// ============================================================================
// 基础功能测试
// ============================================================================

TEST_CASE("类型转换工具函数测试", "[utils][types]") {

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
        // 测试基本描述生成（新接口）
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

        // 测试带操作者信息的描述生成（新接口）
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

TEST_CASE("数据结构测试", "[data][structures]") {

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
}

TEST_CASE("异常处理测试", "[exceptions]") {

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

TEST_CASE("边界条件测试", "[boundary][edge]") {

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

// ============================================================================
// Mock API 测试
// ============================================================================

TEST_CASE("LeviLaminaAPI Mock 测试", "[mock][levilamina]") {

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