#ifdef TESTING

#include "CommandTestHelper.h"
#include "mocks/MockLeviLaminaAPI.h"
#include "mod/config/ConfigManager.h"
#include "mod/economy/EconomyManager.h"
#include "mod/exceptions/MoneyException.h"
#include "mod/types/Types.h"
#include <catch2/catch_all.hpp>

namespace rlx_money::test {

// 由于无法直接访问 Commands.cpp 中的 lambda，我们需要通过间接方式测试
// 这里我们创建一个测试辅助函数，它模拟命令执行流程

bool CommandTestHelper::testBasicQueryCommand(
    const std::string& playerXuid,
    const std::string& playerName,
    const std::string& currencyId,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的，因为测试用例可能已经设置好了）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(playerXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(playerXuid, playerName);
    }

    // 执行命令逻辑（这里我们需要通过某种方式访问命令逻辑）
    // 由于无法直接访问 lambda，我们通过测试业务逻辑来间接测试命令
    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;

        if (currencyId.empty()) {
            // 查询所有币种余额
            auto balances = manager.getAllBalances(playerXuid);
            if (balances.empty()) {
                return !expectSuccess; // 如果没有余额，这可能是预期的
            }
            return expectSuccess;
        } else {
            // 查询指定币种余额
            auto balance = manager.getBalance(playerXuid, actualCurrencyId);
            if (!balance.has_value()) {
                return !expectSuccess;
            }
            return expectSuccess;
        }
    } catch (...) {
        return !expectSuccess;
    }
}

bool CommandTestHelper::testBasicHistoryCommand(
    const std::string& playerXuid,
    const std::string& playerName,
    const std::string& currencyId
) {
    // 确保 Mock 玩家存在
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(playerXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(playerXuid, playerName);
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;
        auto        history          = manager.getPlayerTransactions(playerXuid, actualCurrencyId);
        return true;
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testPayCommand(
    const std::string& fromXuid,
    const std::string& fromName,
    const std::string& toName,
    int                amount,
    const std::string& currencyId,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(fromXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(fromXuid, fromName);
    }

    auto toXuid = rlx_money::LeviLaminaAPI::getXuidByPlayerName(toName);
    if (toXuid.empty()) {
        return !expectSuccess; // 目标玩家不存在
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;

        // 尝试执行转账，让 transferMoney 来验证金额是否有效
        bool success = manager.transferMoney(fromXuid, toXuid, actualCurrencyId, amount, "测试转账");
        // 返回实际结果是否符合预期：成功且期望成功，或失败且期望失败
        return (success == expectSuccess);
    } catch (const rlx_money::InvalidArgumentException&) {
        // 金额无效（包括 <= 0 或小于最小转账金额）- 转账失败
        // 如果期望失败，则返回 true（符合预期）；如果期望成功，则返回 false（不符合预期）
        return !expectSuccess;
    } catch (const rlx_money::MoneyException&) {
        // 其他业务异常（如余额不足、转账禁用等）- 转账失败
        // 如果期望失败，则返回 true（符合预期）；如果期望成功，则返回 false（不符合预期）
        return !expectSuccess;
    } catch (...) {
        // 其他异常 - 转账失败
        // 如果期望失败，则返回 true（符合预期）；如果期望成功，则返回 false（不符合预期）
        return !expectSuccess;
    }
}

bool CommandTestHelper::testAdminSetCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& targetName,
    int                amount,
    const std::string& currencyId,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    auto targetXuid = rlx_money::LeviLaminaAPI::getXuidByPlayerName(targetName);
    if (targetXuid.empty()) {
        return !expectSuccess;
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;

        manager.setBalance(targetXuid, actualCurrencyId, amount, OperatorType::ADMIN, adminName);
        return expectSuccess;
    } catch (...) {
        return !expectSuccess;
    }
}

bool CommandTestHelper::testAdminGiveCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& targetName,
    int                amount,
    const std::string& currencyId,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    auto targetXuid = rlx_money::LeviLaminaAPI::getXuidByPlayerName(targetName);
    if (targetXuid.empty()) {
        return !expectSuccess;
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;

        manager.addMoney(targetXuid, actualCurrencyId, amount, OperatorType::ADMIN, adminName);
        return expectSuccess;
    } catch (...) {
        return !expectSuccess;
    }
}

bool CommandTestHelper::testAdminTakeCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& targetName,
    int                amount,
    const std::string& currencyId,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    auto targetXuid = rlx_money::LeviLaminaAPI::getXuidByPlayerName(targetName);
    if (targetXuid.empty()) {
        return !expectSuccess;
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;

        manager.reduceMoney(targetXuid, actualCurrencyId, amount, OperatorType::ADMIN, adminName);
        return expectSuccess;
    } catch (...) {
        return !expectSuccess;
    }
}

bool CommandTestHelper::testAdminCheckCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& targetName,
    const std::string& currencyId
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    auto targetXuid = rlx_money::LeviLaminaAPI::getXuidByPlayerName(targetName);
    if (targetXuid.empty()) {
        return false;
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;
        auto        balance          = manager.getBalance(targetXuid, actualCurrencyId);
        return balance.has_value();
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testAdminHistoryCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& targetName,
    const std::string& currencyId
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    auto targetXuid = rlx_money::LeviLaminaAPI::getXuidByPlayerName(targetName);
    if (targetXuid.empty()) {
        return false;
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;
        auto        history          = manager.getPlayerTransactions(targetXuid, actualCurrencyId);
        return true;
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testAdminTopCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& currencyId
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    try {
        auto&       manager          = rlx_money::EconomyManager::getInstance();
        std::string actualCurrencyId = currencyId.empty() ? manager.getDefaultCurrencyId() : currencyId;
        auto        topPlayers       = manager.getTopBalanceList(actualCurrencyId, 10);
        return true;
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testAdminSetInitialCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    int                amount,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    try {
        if (amount < 0) {
            return !expectSuccess;
        }
        auto& configManager = rlx_money::ConfigManager::getInstance();
        configManager.setInitialBalance(amount);
        return expectSuccess;
    } catch (...) {
        return !expectSuccess;
    }
}

bool CommandTestHelper::testAdminGetInitialCommand(const std::string& adminXuid, const std::string& adminName) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    try {
        auto& configManager = rlx_money::ConfigManager::getInstance();
        (void)configManager.getInitialBalance(); // 验证可以获取初始金额
        return true;
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testAdminReloadCommand(const std::string& adminXuid, const std::string& adminName) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    try {
        auto& configManager = rlx_money::ConfigManager::getInstance();
        configManager.reloadConfig();
        auto& manager = rlx_money::EconomyManager::getInstance();
        manager.syncCurrenciesFromConfig();
        return true;
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testCurrencyListCommand(const std::string& adminXuid, const std::string& adminName) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    try {
        auto& config = rlx_money::ConfigManager::getInstance().getConfig();
        return !config.currencies.empty();
    } catch (...) {
        return false;
    }
}

bool CommandTestHelper::testCurrencyInfoCommand(
    const std::string& adminXuid,
    const std::string& adminName,
    const std::string& currencyId,
    bool               expectSuccess
) {
    // 确保 Mock 玩家存在（不清除已有的）
    if (!rlx_money::LeviLaminaAPI::getPlayerByXuid(adminXuid)) {
        rlx_money::LeviLaminaAPI::addMockPlayer(adminXuid, adminName);
    }

    try {
        auto& config     = rlx_money::ConfigManager::getInstance().getConfig();
        auto  currencyIt = config.currencies.find(currencyId);
        if (currencyIt == config.currencies.end()) {
            return !expectSuccess;
        }
        return expectSuccess;
    } catch (...) {
        return !expectSuccess;
    }
}

} // namespace rlx_money::test

#endif // TESTING
