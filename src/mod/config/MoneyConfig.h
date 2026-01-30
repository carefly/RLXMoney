#pragma once

#include "ConfigStructures.h"
#include "common/ConfigManager.hpp"

namespace rlx_money {

/// @brief RLXMoney 配置管理类（使用通用配置系统）
///
/// 使用 rlx::common::Config<T> 实现强类型配置管理。
/// 配置文件路径: plugins/RLXModeResources/config/config.json
/// 配置节点名称: RLXMoney
///
/// 使用示例:
/// @code
/// // 初始化配置系统（在插件启动时调用一次）
/// MoneyConfig::initialize();
///
/// // 获取配置
/// const auto& config = MoneyConfig::get();
/// int initialBalance = config.currencies.at(config.defaultCurrency).initialBalance;
///
/// // 重新加载配置
/// MoneyConfig::reload();
///
/// // 保存配置
/// MoneyConfig::save();
///
/// // 重置配置（仅用于测试）
/// MoneyConfig::resetForTesting();
/// @endcode
class MoneyConfig {
public:
    /// @brief 初始化配置系统
    /// @param configPath 配置文件路径（可选，默认为标准路径）
    static void initialize(const std::string& configPath = "plugins/RLXModeResources/config/config.json") {
        // 重置加载状态，强制重新加载配置文件
        rlx::common::ConfigManager::resetLoaded();

        // 设置配置路径和节点
        rlx::common::ConfigManager::setConfigPath(configPath);
        rlx::common::ConfigManager::setModSection("RLXMoney");

        // 重新加载配置文件
        auto& config = getOrCreateConfig();
        config.load();

        // 如果配置为空（文件不存在或为空），则创建默认配置
        if (config.get().currencies.empty()) {
            createDefaultConfig();
        } else {
            // 配置已加载，但文件可能只包含部分字段。
            // 保存配置以确保文件包含所有字段（包括默认值）
            config.save();
        }

        // 验证配置
        config.get().validate();
    }

    /// @brief 获取配置（只读）
    static const MoneyConfigData& get() {
        return getOrCreateConfig().get();
    }

    /// @brief 获取配置（可写）- 修改后需要调用 save() 保存
    static MoneyConfigData& getWritable() {
        return getOrCreateConfig().get();
    }

    /// @brief 保存配置到文件
    static void save() {
        // 保存前验证配置
        getWritable().validate();
        getOrCreateConfig().save();
    }

    /// @brief 重新加载配置
    static void reload() {
        // 重置加载状态，强制重新读取文件
        rlx::common::ConfigManager::resetLoaded();
        getOrCreateConfig().load();
        // 重新加载后验证配置
        get().validate();
    }

    /// @brief 获取配置文件路径
    static const std::string& getConfigPath() {
        return rlx::common::ConfigManager::getConfigPath();
    }

    /// @brief 设置初始金额（默认币种）
    static void setInitialBalance(int amount) {
        auto& config = getWritable();
        if (config.currencies.find(config.defaultCurrency) == config.currencies.end()) {
            throw std::runtime_error("默认币种不存在");
        }
        auto& currency = config.currencies[config.defaultCurrency];
        if (amount < 0 || amount > currency.maxBalance) {
            throw std::runtime_error("初始金额不合法");
        }
        currency.initialBalance = amount;
        save();
    }

    /// @brief 获取初始金额（默认币种）
    static int getInitialBalance() {
        const auto& config = get();
        if (config.currencies.find(config.defaultCurrency) == config.currencies.end()) {
            return 1000; // 默认值
        }
        return config.currencies.at(config.defaultCurrency).initialBalance;
    }

    /// @brief 设置玩家转账是否允许（默认币种）
    static void setAllowPlayerTransfer(bool allow) {
        auto& config = getWritable();
        if (config.currencies.find(config.defaultCurrency) == config.currencies.end()) {
            throw std::runtime_error("默认币种不存在");
        }
        config.currencies[config.defaultCurrency].allowPlayerTransfer = allow;
        save();
    }

    /// @brief 获取玩家转账是否允许（默认币种）
    static bool getAllowPlayerTransfer() {
        const auto& config = get();
        if (config.currencies.find(config.defaultCurrency) == config.currencies.end()) {
            return true; // 默认值
        }
        return config.currencies.at(config.defaultCurrency).allowPlayerTransfer;
    }

    /// @brief 重置配置（仅用于测试）
    static void resetForTesting() {
        // 重置 ConfigManager 的加载状态
        rlx::common::ConfigManager::resetLoaded();
        // 重置配置对象
        getOrCreateConfig().reset();
    }

private:
    /// @brief 获取或创建配置对象（内部使用）
    static rlx::common::Config<MoneyConfigData>& getOrCreateConfig() {
        static rlx::common::Config<MoneyConfigData> config("RLXMoney");
        return config;
    }

    /// @brief 获取配置引用（用于重置）
    static rlx::common::Config<MoneyConfigData>& getConfigRef() {
        return getOrCreateConfig();
    }

    /// @brief 创建默认配置
    static void createDefaultConfig() {
        auto& config = getWritable();

        // 创建默认币种 "gold"
        Currency gold;
        gold.currencyId          = "gold";
        gold.name                = "金币";
        gold.symbol              = "G";
        gold.displayFormat       = "{amount} {symbol}";
        gold.enabled             = true;
        gold.initialBalance      = 1000;
        gold.maxBalance          = std::numeric_limits<int>::max(); // 0 表示无限制，内部使用 INT_MAX
        gold.minTransferAmount   = 1;
        gold.transferFee         = 0;
        gold.feePercentage       = 0.0;
        gold.allowPlayerTransfer = true;

        config.defaultCurrency    = "gold";
        config.currencies["gold"] = gold;

        save();
    }
};

/// @brief 便捷函数：获取金钱配置
inline const MoneyConfigData& getMoneyConfig() {
    return MoneyConfig::get();
}

} // namespace rlx_money
