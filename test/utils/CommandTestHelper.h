#pragma once

#ifdef TESTING

#include <string>

namespace rlx_money::test {

/// @brief 命令测试辅助类，用于在不修改源代码的情况下测试命令逻辑
class CommandTestHelper {
public:
    /// @brief 测试 BasicCommand 的查询操作
    static bool testBasicQueryCommand(
        const std::string& playerXuid,
        const std::string& playerName,
        const std::string& currencyId    = "",
        bool               expectSuccess = true
    );

    /// @brief 测试 BasicCommand 的历史记录查询操作
    static bool testBasicHistoryCommand(
        const std::string& playerXuid,
        const std::string& playerName,
        const std::string& currencyId = ""
    );

    /// @brief 测试 PayCommand 的转账操作
    static bool testPayCommand(
        const std::string& fromXuid,
        const std::string& fromName,
        const std::string& toName,
        int                amount,
        const std::string& currencyId    = "",
        bool               expectSuccess = true
    );

    /// @brief 测试 AdminCommand 的设置余额操作
    static bool testAdminSetCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        const std::string& targetName,
        int                amount,
        const std::string& currencyId    = "",
        bool               expectSuccess = true
    );

    /// @brief 测试 AdminCommand 的给予操作
    static bool testAdminGiveCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        const std::string& targetName,
        int                amount,
        const std::string& currencyId    = "",
        bool               expectSuccess = true
    );

    /// @brief 测试 AdminCommand 的扣除操作
    static bool testAdminTakeCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        const std::string& targetName,
        int                amount,
        const std::string& currencyId    = "",
        bool               expectSuccess = true
    );

    /// @brief 测试 AdminCommand 的查询操作
    static bool testAdminCheckCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        const std::string& targetName,
        const std::string& currencyId = ""
    );

    /// @brief 测试 AdminCommand 的历史记录查询操作
    static bool testAdminHistoryCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        const std::string& targetName,
        const std::string& currencyId = ""
    );

    /// @brief 测试 AdminCommand 的排行榜操作
    static bool
    testAdminTopCommand(const std::string& adminXuid, const std::string& adminName, const std::string& currencyId = "");

    /// @brief 测试 AdminCommand 的设置初始金额操作
    static bool testAdminSetInitialCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        int                amount,
        bool               expectSuccess = true
    );

    /// @brief 测试 AdminCommand 的获取初始金额操作
    static bool testAdminGetInitialCommand(const std::string& adminXuid, const std::string& adminName);

    /// @brief 测试 AdminCommand 的重载配置操作
    static bool testAdminReloadCommand(const std::string& adminXuid, const std::string& adminName);

    /// @brief 测试 CurrencyCommand 的列表操作
    static bool testCurrencyListCommand(const std::string& adminXuid, const std::string& adminName);

    /// @brief 测试 CurrencyCommand 的信息查询操作
    static bool testCurrencyInfoCommand(
        const std::string& adminXuid,
        const std::string& adminName,
        const std::string& currencyId,
        bool               expectSuccess = true
    );
};

} // namespace rlx_money::test

#endif // TESTING
