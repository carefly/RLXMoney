#pragma once

#include <map>
#include <string>

namespace rlx_money {

/// @brief 数据库配置结构
struct DatabaseConfig {
    std::string path = "plugins/RLXModeResources/data/money/money.db";

    struct {
        bool        walMode     = true;
        int         cacheSize   = 2000;
        std::string synchronous = "NORMAL";
    } optimization;
};

/// @brief 币种结构（包含显示信息和业务配置）
struct Currency {
    // 基本信息
    std::string currencyId;    // 币种ID
    std::string name;          // 币种名称
    std::string symbol;        // 币种符号
    std::string displayFormat; // 显示格式模板
    bool        enabled;       // 是否启用

    // 业务配置
    int initialBalance      = 1000;
    int maxBalance          = 0;
    int minTransferAmount   = 1;
    int transferFee         = 0;
    double  feePercentage       = 0.0;
    bool    allowPlayerTransfer = true;

    Currency() : displayFormat("{amount} {symbol}"), enabled(true) {}
};

/// @brief 排行榜配置结构
struct TopListConfig {
    int defaultCount = 10;
    int maxCount     = 50;
};

/// @brief 主配置结构
struct ModConfig {
    DatabaseConfig                  database;
    std::string                     defaultCurrency = "gold";
    std::map<std::string, Currency> currencies; // 币种ID -> 币种
    TopListConfig                   topList;
};

} // namespace rlx_money