#pragma once

#include "ConfigStructures.h"
#include "common/ConfigManager.hpp"

namespace rlx_money {

/// @brief RLXMoney 配置管理类（使用通用配置系统）
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
/// // 修改配置
/// MoneyConfig::getWritable().currencies["gold"].initialBalance = 2000;
/// MoneyConfig::save();
///
/// // 重新加载配置
/// MoneyConfig::reload();
///
/// // 重置配置（仅用于测试）
/// MoneyConfig::resetForTesting();
/// @endcode
class MoneyConfig {
public:
    /// @brief 初始化配置系统
    /// @param fileNameOrPath 配置文件名或完整路径
    ///                      - 文件名（不含路径分隔符）: 使用固定前缀 plugins/RLXModeResources/config/
    ///                      - 完整路径（含路径分隔符）: 直接使用（用于测试）
    ///                      - 默认值: "rlx_money.json" -> 实际路径: plugins/RLXModeResources/config/rlx_money.json
    static void initialize(const std::string& fileNameOrPath = "rlx_money.json") {
        // 检测是否包含路径分隔符
        bool isFullPath = fileNameOrPath.find('/') != std::string::npos ||
                          fileNameOrPath.find('\\') != std::string::npos;

        if (isFullPath) {
            rlx::common::Config<MoneyConfigData>::init(fileNameOrPath);
        } else {
            rlx::common::Config<MoneyConfigData>::initWithName(fileNameOrPath);
        }
        auto& config = rlx::common::Config<MoneyConfigData>::getInstance();

        // 如果配置为空，创建默认配置
        if (config->currencies.empty()) {
            Currency gold;
            gold.currencyId          = "gold";
            gold.name                = "金币";
            gold.symbol              = "G";
            gold.enabled             = true;
            gold.initialBalance      = 1000;
            gold.maxBalance          = std::numeric_limits<int>::max();
            gold.minTransferAmount   = 1;
            gold.transferFee         = 0;
            gold.feePercentage       = 0.0;
            gold.allowPlayerTransfer = true;

            config->defaultCurrency    = "gold";
            config->currencies["gold"] = gold;
        }

        config->validate();
        config.save();
    }

    /// @brief 获取配置（只读）
    static const MoneyConfigData& get() {
        return rlx::common::Config<MoneyConfigData>::getInstance().get();
    }

    /// @brief 获取配置（可写）
    static MoneyConfigData& getWritable() {
        return rlx::common::Config<MoneyConfigData>::getInstance().get();
    }

    /// @brief 保存配置
    static void save() {
        auto& config = rlx::common::Config<MoneyConfigData>::getInstance();
        config->validate();
        config.save();
    }

    /// @brief 重新加载配置
    static void reload() {
        rlx::common::Config<MoneyConfigData>::getInstance().reload();
        get().validate();
    }

    /// @brief 获取配置文件路径
    static const std::string& getConfigPath() {
        return rlx::common::Config<MoneyConfigData>::getInstance().getPath();
    }

    /// @brief 重置配置（仅用于测试）
    static void resetForTesting() {
        rlx::common::Config<MoneyConfigData>::reset();
    }
};

/// @brief 便捷函数：获取金钱配置
inline const MoneyConfigData& getMoneyConfig() { return MoneyConfig::get(); }

} // namespace rlx_money
