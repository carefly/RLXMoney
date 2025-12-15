#include "mod/api/RLXMoneyAPI.h"
#include "mod/config/ConfigManager.h"
#include "mod/database/DatabaseManager.h"
#include "mod/economy/EconomyManager.h"


namespace rlx_money {

std::optional<int> RLXMoneyAPI::getBalance(const std::string& xuid, const std::string& currencyId) {
    return EconomyManager::getInstance().getBalance(xuid, currencyId);
}

std::vector<PlayerBalance> RLXMoneyAPI::getAllBalances(const std::string& xuid) {
    return EconomyManager::getInstance().getAllBalances(xuid);
}

bool RLXMoneyAPI::setBalance(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    return EconomyManager::getInstance().setBalance(xuid, currencyId, amount, description);
}

bool RLXMoneyAPI::addMoney(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    return EconomyManager::getInstance().addMoney(xuid, currencyId, amount, description);
}

bool RLXMoneyAPI::reduceMoney(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    return EconomyManager::getInstance().reduceMoney(xuid, currencyId, amount, description);
}

bool RLXMoneyAPI::playerExists(const std::string& xuid) { return EconomyManager::getInstance().playerExists(xuid); }

bool RLXMoneyAPI::transferMoney(
    const std::string& fromXuid,
    const std::string& toXuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    return EconomyManager::getInstance().transferMoney(fromXuid, toXuid, currencyId, amount, description);
}

bool RLXMoneyAPI::hasSufficientBalance(const std::string& xuid, const std::string& currencyId, int amount) {
    return EconomyManager::getInstance().hasSufficientBalance(xuid, currencyId, amount);
}

std::vector<TopBalanceEntry> RLXMoneyAPI::getTopBalanceList(const std::string& currencyId, int limit) {
    return EconomyManager::getInstance().getTopBalanceList(currencyId, limit);
}

std::vector<TransactionRecord>
RLXMoneyAPI::getPlayerTransactions(const std::string& xuid, const std::string& currencyId, int page, int pageSize) {
    return EconomyManager::getInstance().getPlayerTransactions(xuid, currencyId, page, pageSize);
}

int RLXMoneyAPI::getPlayerTransactionCount(const std::string& xuid) {
    return EconomyManager::getInstance().getPlayerTransactionCount(xuid);
}

int RLXMoneyAPI::getTotalWealth(const std::string& currencyId) {
    return EconomyManager::getInstance().getTotalWealth(currencyId);
}

int RLXMoneyAPI::getPlayerCount() { return EconomyManager::getInstance().getPlayerCount(); }

bool RLXMoneyAPI::isValidAmount(int amount) { return EconomyManager::getInstance().isValidAmount(amount); }

std::vector<std::string> RLXMoneyAPI::getEnabledCurrencyIds() {
    auto&                    config = ConfigManager::getInstance().getConfig();
    std::vector<std::string> result;
    for (const auto& [currencyId, currency] : config.currencies) {
        if (currency.enabled) {
            result.push_back(currencyId);
        }
    }
    return result;
}

std::string RLXMoneyAPI::getDefaultCurrencyId() { return ConfigManager::getInstance().getConfig().defaultCurrency; }

bool RLXMoneyAPI::initialize(const std::string& configPath) {
    try {
        // 1. 加载配置
        ConfigManager::getInstance().loadConfig(configPath);

        // 2. 初始化数据库
        auto& config = ConfigManager::getInstance().getConfig();
        if (!DatabaseManager::getInstance().initialize(config.database.path)) {
            return false;
        }

        // 3. 初始化经济管理器
        if (!EconomyManager::getInstance().initialize()) {
            return false;
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RLXMoneyAPI::isInitialized() {
    try {
        // 检查数据库是否已初始化（这是最关键的检查）
        if (!DatabaseManager::getInstance().isInitialized()) {
            return false;
        }

        // 检查配置是否已加载（通过检查是否有默认币种）
        auto& config = ConfigManager::getInstance().getConfig();
        if (config.defaultCurrency.empty() || config.currencies.empty()) {
            return false;
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace rlx_money