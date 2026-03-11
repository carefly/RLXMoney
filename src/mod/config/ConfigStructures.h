#pragma once

#include <limits>
#include <map>
#include <string>
#include <nlohmann/json.hpp>
#include "common/ConfigManager.hpp"

namespace rlx_money {

// ==================== 前向声明 ====================
struct DatabaseConfig;
struct Currency;
struct ModConfig;

// ==================== 序列化函数声明 ====================
void to_json(nlohmann::json& j, const DatabaseConfig& db);
void from_json(const nlohmann::json& j, DatabaseConfig& db);

void to_json(nlohmann::json& j, const Currency& c);
void from_json(const nlohmann::json& j, Currency& c);

void to_json(nlohmann::json& j, const ModConfig& config);
void from_json(const nlohmann::json& j, ModConfig& config);

// ==================== 结构体定义 ====================

/// @brief 数据库配置结构
struct DatabaseConfig {
    std::string path = "plugins/RLXModeResources/data/money/money.db";

    /// @brief 验证数据库配置
    void validate() const;
};

/// @brief 币种结构（包含显示信息和业务配置）
struct Currency {
    // 基本信息
    std::string currencyId; // 币种ID
    std::string name;       // 币种名称
    std::string symbol;     // 币种符号
    bool        enabled;    // 是否启用

    // 业务配置
    int  initialBalance      = 1000;
    int  maxBalance          = 0;
    int  minTransferAmount   = 1;
    int  transferFee         = 0;
    double feePercentage       = 0.0;
    bool   allowPlayerTransfer = true;

    Currency() : enabled(true) {
        // maxBalance 默认为 0，表示无限制（内部使用 INT_MAX 表示）
        maxBalance = std::numeric_limits<int>::max();
    }

    /// @brief 验证币种配置的有效性
    /// @throw std::invalid_argument 如果配置无效
    void validate() const;
};

/// @brief 主配置结构
struct ModConfig {
    DatabaseConfig                  database;
    std::string                     defaultCurrency = "gold";
    std::map<std::string, Currency> currencies; // 币种ID -> 币种

    /// @brief 默认构造函数：创建默认金币配置
    ModConfig() {
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

        currencies["gold"] = gold;
    }

    /// @brief 验证整体配置的有效性
    /// @throw std::invalid_argument 如果配置无效
    void validate() const;
};

/// @brief MoneyConfigData 类型别名（符合通用配置系统规范）
using MoneyConfigData = ModConfig;

// ==================== 序列化函数实现 ====================

/// @brief DatabaseConfig 的自定义序列化（带类型验证）
inline void to_json(nlohmann::json& j, const DatabaseConfig& db) {
    j["path"] = db.path;
}

inline void from_json(const nlohmann::json& j, DatabaseConfig& db) {
    if (j.contains("path")) {
        if (!j["path"].is_string()) {
            throw std::invalid_argument("database.path 必须是字符串类型");
        }
        j.at("path").get_to(db.path);
    }
}

/// @brief Currency 的自定义序列化（处理 maxBalance=0 表示无限制）
inline void to_json(nlohmann::json& j, const Currency& c) {
    j["currencyId"] = c.currencyId;
    j["name"] = c.name;
    j["symbol"] = c.symbol;
    j["enabled"] = c.enabled;
    j["initialBalance"] = c.initialBalance;
    j["maxBalance"] = c.maxBalance == std::numeric_limits<int>::max() ? 0 : c.maxBalance; // INT_MAX 转为 0
    j["minTransferAmount"] = c.minTransferAmount;
    j["transferFee"] = c.transferFee;
    j["feePercentage"] = c.feePercentage;
    j["allowPlayerTransfer"] = c.allowPlayerTransfer;
}

/// @brief Currency 的自定义反序列化（处理 maxBalance=0 表示无限制）
inline void from_json(const nlohmann::json& j, Currency& c) {
    // 验证字段类型并提供详细错误信息
    if (j.contains("currencyId") && !j["currencyId"].is_null()) {
        if (!j["currencyId"].is_string()) {
            throw std::invalid_argument("currencyId 必须是字符串类型");
        }
        j.at("currencyId").get_to(c.currencyId);
    }

    if (j.contains("name")) {
        if (!j["name"].is_string()) {
            throw std::invalid_argument("name 必须是字符串类型");
        }
        j.at("name").get_to(c.name);
    }

    if (j.contains("symbol")) {
        if (!j["symbol"].is_string()) {
            throw std::invalid_argument("symbol 必须是字符串类型");
        }
        j.at("symbol").get_to(c.symbol);
    }

    if (j.contains("enabled")) {
        if (!j["enabled"].is_boolean()) {
            throw std::invalid_argument("enabled 必须是布尔类型");
        }
        j.at("enabled").get_to(c.enabled);
    }

    if (j.contains("initialBalance")) {
        if (!j["initialBalance"].is_number_integer()) {
            throw std::invalid_argument("initialBalance 必须是整数类型");
        }
        j.at("initialBalance").get_to(c.initialBalance);
        if (c.initialBalance < 0) {
            throw std::invalid_argument("initialBalance 不能为负数");
        }
    }

    if (j.contains("maxBalance")) {
        if (!j["maxBalance"].is_number_integer()) {
            throw std::invalid_argument("maxBalance 必须是整数类型");
        }
        int maxBalanceValue = j["maxBalance"].get<int>();
        // 0 表示无限制，内部使用 INT_MAX 表示
        c.maxBalance = (maxBalanceValue == 0) ? std::numeric_limits<int>::max() : maxBalanceValue;
        if (maxBalanceValue < 0) {
            throw std::invalid_argument("maxBalance 不能为负数");
        }
    }

    if (j.contains("minTransferAmount")) {
        if (!j["minTransferAmount"].is_number_integer()) {
            throw std::invalid_argument("minTransferAmount 必须是整数类型");
        }
        j.at("minTransferAmount").get_to(c.minTransferAmount);
        if (c.minTransferAmount < 0) {
            throw std::invalid_argument("minTransferAmount 不能为负数");
        }
    }

    if (j.contains("transferFee")) {
        if (!j["transferFee"].is_number_integer()) {
            throw std::invalid_argument("transferFee 必须是整数类型");
        }
        j.at("transferFee").get_to(c.transferFee);
        if (c.transferFee < 0) {
            throw std::invalid_argument("transferFee 不能为负数");
        }
    }

    if (j.contains("feePercentage")) {
        if (!j["feePercentage"].is_number()) {
            throw std::invalid_argument("feePercentage 必须是数字类型");
        }
        j.at("feePercentage").get_to(c.feePercentage);
        if (c.feePercentage < 0.0 || c.feePercentage > 100.0) {
            throw std::invalid_argument("feePercentage 必须在 0.0 到 100.0 之间");
        }
    }

    if (j.contains("allowPlayerTransfer")) {
        if (!j["allowPlayerTransfer"].is_boolean()) {
            throw std::invalid_argument("allowPlayerTransfer 必须是布尔类型");
        }
        j.at("allowPlayerTransfer").get_to(c.allowPlayerTransfer);
    }
}

/// @brief ModConfig 的自定义序列化（带类型验证）
inline void to_json(nlohmann::json& j, const ModConfig& config) {
    j["database"] = config.database;
    j["defaultCurrency"] = config.defaultCurrency;
    j["currencies"] = config.currencies;
}

inline void from_json(const nlohmann::json& j, ModConfig& config) {
    if (j.contains("database")) {
        if (!j["database"].is_object()) {
            throw std::invalid_argument("database 必须是对象类型");
        }
        j.at("database").get_to(config.database);
    }

    if (j.contains("defaultCurrency")) {
        if (!j["defaultCurrency"].is_string()) {
            throw std::invalid_argument("defaultCurrency 必须是字符串类型");
        }
        j.at("defaultCurrency").get_to(config.defaultCurrency);
    }

    if (j.contains("currencies")) {
        if (!j["currencies"].is_object()) {
            throw std::invalid_argument("currencies 必须是对象类型");
        }
        config.currencies.clear();
        for (const auto& [currencyId, currencyData] : j["currencies"].items()) {
            if (!currencyData.is_object()) {
                throw std::invalid_argument("币种 " + currencyId + " 必须是对象类型");
            }
            Currency currency;
            from_json(currencyData, currency); // 使用显式调用避免 ADL 问题
            currency.currencyId = currencyId; // 确保货币ID与key一致
            config.currencies[currencyId] = currency;
        }
    }
}

// ==================== Validate 方法实现 ====================

inline void DatabaseConfig::validate() const {
    if (path.empty()) {
        throw std::invalid_argument("database.path 不能为空");
    }
}

inline void Currency::validate() const {
    if (initialBalance < 0) {
        throw std::invalid_argument("币种 " + currencyId + " 的 initialBalance 不能为负数");
    }
    if (minTransferAmount < 0) {
        throw std::invalid_argument("币种 " + currencyId + " 的 minTransferAmount 不能为负数");
    }
    if (transferFee < 0) {
        throw std::invalid_argument("币种 " + currencyId + " 的 transferFee 不能为负数");
    }
    if (feePercentage < 0.0 || feePercentage > 100.0) {
        throw std::invalid_argument("币种 " + currencyId + " 的 feePercentage 必须在 0.0 到 100.0 之间");
    }
}

inline void ModConfig::validate() const {
    database.validate();

    // 验证默认币种存在
    if (currencies.empty()) {
        throw std::invalid_argument("配置中必须至少有一个币种");
    }
    if (currencies.find(defaultCurrency) == currencies.end()) {
        throw std::invalid_argument("默认币种 '" + defaultCurrency + "' 在 currencies 中不存在");
    }

    // 验证每个币种
    for (const auto& [currencyId, currency] : currencies) {
        currency.validate();
    }
}

} // namespace rlx_money

// ==================== 类型别名 ====================

namespace rlx_money {

/// @brief 便捷类型别名：金钱配置数据
using MoneyConfigData = ModConfig;

// 导入通用配置模板
using rlx::common::Config;

/// @brief 金钱配置类型别名
/// 使用方式：MoneyConfig::init(path), MoneyConfig::get(), MoneyConfig::getInstance()
using MoneyConfig = Config<MoneyConfigData>;

} // namespace rlx_money
