#include "mod/config/ConfigManager.h"
#include "mod/exceptions/MoneyException.h"
#include <filesystem>
#include <fstream>
#include <limits>
#include <nlohmann/json.hpp>


using json = nlohmann::json;

namespace rlx_money {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::loadConfig(const std::string& configPath) {
    try {
        mConfigPath = configPath;

        // 确保配置目录存在
        ensureConfigDirectory(configPath);

        std::ifstream file(configPath);
        if (!file.is_open()) {
            // 配置文件不存在，创建默认配置
            mConfig = createDefaultConfig();
            saveConfig();
            return;
        }

        // 使用 json::parse 进行更好的错误处理
        json j;
        try {
            j = json::parse(file);
        } catch (const json::parse_error& e) {
            throw ConfigException("JSON 格式错误: " + std::string(e.what()));
        }

        // 创建临时配置对象用于解析
        ModConfig tempConfig = mConfig; // 保留当前配置作为备份

        // 解析配置
        if (j.contains("database")) {
            const auto& db = j["database"];
            if (db.contains("path")) {
                if (!db["path"].is_string()) {
                    throw ConfigException("database.path 必须是字符串");
                }
                tempConfig.database.path = db["path"];
            }
            if (db.contains("optimization")) {
                const auto& opt = db["optimization"];
                if (opt.contains("wal_mode")) {
                    if (!opt["wal_mode"].is_boolean()) {
                        throw ConfigException("database.optimization.wal_mode 必须是布尔值");
                    }
                    tempConfig.database.optimization.walMode = opt["wal_mode"];
                }
                if (opt.contains("cache_size")) {
                    if (!opt["cache_size"].is_number_integer()) {
                        throw ConfigException("database.optimization.cache_size 必须是整数");
                    }
                    tempConfig.database.optimization.cacheSize = opt["cache_size"];
                }
                if (opt.contains("synchronous")) {
                    if (!opt["synchronous"].is_string()) {
                        throw ConfigException("database.optimization.synchronous 必须是字符串");
                    }
                    tempConfig.database.optimization.synchronous = opt["synchronous"];
                }
            }
        }

        // 解析默认币种
        if (j.contains("defaultCurrency")) {
            if (!j["defaultCurrency"].is_string()) {
                throw ConfigException("defaultCurrency 必须是字符串");
            }
            tempConfig.defaultCurrency = j["defaultCurrency"];
        }

        // 解析币种配置
        if (j.contains("currencies")) {
            if (!j["currencies"].is_object()) {
                throw ConfigException("currencies 必须是对象");
            }

            tempConfig.currencies.clear();

            for (const auto& [currencyId, currencyData] : j["currencies"].items()) {
                if (!currencyData.is_object()) {
                    throw ConfigException("币种配置 " + currencyId + " 必须是对象");
                }

                Currency currency;
                currency.currencyId = currencyId;

                // 基本信息
                if (currencyData.contains("name")) {
                    if (!currencyData["name"].is_string()) {
                        throw ConfigException("币种 " + currencyId + " 的 name 必须是字符串");
                    }
                    currency.name = currencyData["name"];
                } else {
                    currency.name = currencyId;
                }

                if (currencyData.contains("symbol")) {
                    if (!currencyData["symbol"].is_string()) {
                        throw ConfigException("币种 " + currencyId + " 的 symbol 必须是字符串");
                    }
                    currency.symbol = currencyData["symbol"];
                } else {
                    currency.symbol = currencyId;
                }

                if (currencyData.contains("displayFormat")) {
                    if (!currencyData["displayFormat"].is_string()) {
                        throw ConfigException("币种 " + currencyId + " 的 displayFormat 必须是字符串");
                    }
                    currency.displayFormat = currencyData["displayFormat"];
                }

                if (currencyData.contains("enabled")) {
                    if (!currencyData["enabled"].is_boolean()) {
                        throw ConfigException("币种 " + currencyId + " 的 enabled 必须是布尔值");
                    }
                    currency.enabled = currencyData["enabled"];
                }

                // 业务配置
                if (currencyData.contains("initialBalance")) {
                    if (!currencyData["initialBalance"].is_number_integer()) {
                        throw ConfigException("币种 " + currencyId + " 的 initialBalance 必须是整数");
                    }
                    currency.initialBalance = currencyData["initialBalance"];
                }

                if (currencyData.contains("maxBalance")) {
                    if (!currencyData["maxBalance"].is_number_integer()) {
                        throw ConfigException("币种 " + currencyId + " 的 maxBalance 必须是整数");
                    }
                    int maxBalanceValue = currencyData["maxBalance"];
                    // 如果 maxBalance 配置为 0，则限制为 int 的最大值
                    currency.maxBalance = (maxBalanceValue == 0) ? std::numeric_limits<int>::max() : maxBalanceValue;
                } else {
                    // 如果配置文件中没有 maxBalance，默认设置为 INT_MAX（无限制）
                    currency.maxBalance = std::numeric_limits<int>::max();
                }

                if (currencyData.contains("minTransferAmount")) {
                    if (!currencyData["minTransferAmount"].is_number_integer()) {
                        throw ConfigException("币种 " + currencyId + " 的 minTransferAmount 必须是整数");
                    }
                    currency.minTransferAmount = currencyData["minTransferAmount"];
                }

                if (currencyData.contains("transferFee")) {
                    if (!currencyData["transferFee"].is_number_integer()) {
                        throw ConfigException("币种 " + currencyId + " 的 transferFee 必须是整数");
                    }
                    currency.transferFee = currencyData["transferFee"];
                }

                if (currencyData.contains("feePercentage")) {
                    if (!currencyData["feePercentage"].is_number()) {
                        throw ConfigException("币种 " + currencyId + " 的 feePercentage 必须是数字");
                    }
                    currency.feePercentage = currencyData["feePercentage"];
                }

                if (currencyData.contains("allowPlayerTransfer")) {
                    if (!currencyData["allowPlayerTransfer"].is_boolean()) {
                        throw ConfigException("币种 " + currencyId + " 的 allowPlayerTransfer 必须是布尔值");
                    }
                    currency.allowPlayerTransfer = currencyData["allowPlayerTransfer"];
                }

                tempConfig.currencies[currencyId] = currency;
            }
        }

        if (j.contains("top_list")) {
            const auto& top = j["top_list"];
            if (top.contains("default_count")) {
                if (!top["default_count"].is_number_integer()) {
                    throw ConfigException("top_list.default_count 必须是整数");
                }
                tempConfig.topList.defaultCount = top["default_count"];
            }
            if (top.contains("max_count")) {
                if (!top["max_count"].is_number_integer()) {
                    throw ConfigException("top_list.max_count 必须是整数");
                }
                tempConfig.topList.maxCount = top["max_count"];
            }
        }

        // 验证配置
        validateConfig(tempConfig);
        mConfig = tempConfig; // 验证通过后才赋值

        // 保存完整的配置文件（包含缺失的默认配置项）
        saveConfig();
        return;

    } catch (const std::exception& e) {
        throw ConfigException("加载配置文件失败: " + std::string(e.what()));
    }
}

void ConfigManager::reloadConfig() { loadConfig(mConfigPath); }

const ModConfig& ConfigManager::getConfig() const { return mConfig; }

void ConfigManager::saveConfig() const {
    try {
        json j;

        // 数据库配置
        j["database"]["path"]                        = mConfig.database.path;
        j["database"]["optimization"]["wal_mode"]    = mConfig.database.optimization.walMode;
        j["database"]["optimization"]["cache_size"]  = mConfig.database.optimization.cacheSize;
        j["database"]["optimization"]["synchronous"] = mConfig.database.optimization.synchronous;

        // 默认币种
        j["defaultCurrency"] = mConfig.defaultCurrency;

        // 币种配置
        for (const auto& [currencyId, currency] : mConfig.currencies) {
            j["currencies"][currencyId]["name"]           = currency.name;
            j["currencies"][currencyId]["symbol"]         = currency.symbol;
            j["currencies"][currencyId]["displayFormat"]  = currency.displayFormat;
            j["currencies"][currencyId]["enabled"]        = currency.enabled;
            j["currencies"][currencyId]["initialBalance"] = currency.initialBalance;
            // 如果 maxBalance 是 int 的最大值，保存为 0 以保持配置文件可读性
            j["currencies"][currencyId]["maxBalance"] =
                (currency.maxBalance == std::numeric_limits<int>::max()) ? 0 : currency.maxBalance;
            j["currencies"][currencyId]["minTransferAmount"]   = currency.minTransferAmount;
            j["currencies"][currencyId]["transferFee"]         = currency.transferFee;
            j["currencies"][currencyId]["feePercentage"]       = currency.feePercentage;
            j["currencies"][currencyId]["allowPlayerTransfer"] = currency.allowPlayerTransfer;
        }

        // 排行榜配置
        j["top_list"]["default_count"] = mConfig.topList.defaultCount;
        j["top_list"]["max_count"]     = mConfig.topList.maxCount;

        std::ofstream file(mConfigPath);
        if (!file.is_open()) {
            throw ConfigException("保存配置文件失败: 无法打开文件");
        }

        file << j.dump(4);
        return;

    } catch (const std::exception& e) {
        throw ConfigException("保存配置文件失败: " + std::string(e.what()));
    }
}

const std::string& ConfigManager::getConfigPath() const { return mConfigPath; }

void ConfigManager::setInitialBalance(int amount) {
    // 设置默认币种的初始余额
    if (mConfig.currencies.find(mConfig.defaultCurrency) == mConfig.currencies.end()) {
        throw ConfigException("默认币种不存在");
    }
    auto& currency = mConfig.currencies[mConfig.defaultCurrency];
    if (amount < 0 || amount > currency.maxBalance) {
        throw ConfigException("初始金额不合法");
    }
    currency.initialBalance = amount;
    saveConfig();
}

int ConfigManager::getInitialBalance() const {
    if (mConfig.currencies.find(mConfig.defaultCurrency) == mConfig.currencies.end()) {
        return 1000; // 默认值
    }
    return mConfig.currencies.at(mConfig.defaultCurrency).initialBalance;
}

void ConfigManager::setAllowPlayerTransfer(bool allow) {
    // 设置默认币种的转账权限
    if (mConfig.currencies.find(mConfig.defaultCurrency) == mConfig.currencies.end()) {
        throw ConfigException("默认币种不存在");
    }
    mConfig.currencies[mConfig.defaultCurrency].allowPlayerTransfer = allow;
    saveConfig();
}

bool ConfigManager::getAllowPlayerTransfer() const {
    if (mConfig.currencies.find(mConfig.defaultCurrency) == mConfig.currencies.end()) {
        return true; // 默认值
    }
    return mConfig.currencies.at(mConfig.defaultCurrency).allowPlayerTransfer;
}

ModConfig ConfigManager::createDefaultConfig() const {
    ModConfig config;

    // 创建默认币种 "gold"
    Currency gold;
    gold.currencyId          = "gold";
    gold.name                = "金币";
    gold.symbol              = "G";
    gold.displayFormat       = "{amount} {symbol}";
    gold.enabled             = true;
    gold.initialBalance      = 1000;
    gold.maxBalance          = 0;
    gold.minTransferAmount   = 1;
    gold.transferFee         = 0;
    gold.feePercentage       = 0.0;
    gold.allowPlayerTransfer = true;

    config.defaultCurrency    = "gold";
    config.currencies["gold"] = gold;

    return config;
}

void ConfigManager::validateConfig(const ModConfig& config) const {
    // 验证数据库配置
    if (config.database.path.empty()) {
        throw ConfigException("数据库路径不能为空");
    }

    // 验证币种配置
    if (config.currencies.empty()) {
        throw ConfigException("至少需要配置一个币种");
    }

    if (config.currencies.find(config.defaultCurrency) == config.currencies.end()) {
        throw ConfigException("默认币种 " + config.defaultCurrency + " 不存在");
    }

    for (const auto& [currencyId, currency] : config.currencies) {
        if (currencyId.empty()) {
            throw ConfigException("币种ID不能为空");
        }

        if (currency.name.empty()) {
            throw ConfigException("币种 " + currencyId + " 的名称不能为空");
        }

        if (currency.symbol.empty()) {
            throw ConfigException("币种 " + currencyId + " 的符号不能为空");
        }

        if (currency.initialBalance < 0) {
            throw ConfigException("币种 " + currencyId + " 的初始金额不能为负数");
        }

        if (currency.maxBalance < 0) {
            throw ConfigException("币种 " + currencyId + " 的最大金额不能为负数");
        }

        if (currency.initialBalance > currency.maxBalance) {
            throw ConfigException("币种 " + currencyId + " 的初始金额不能大于最大金额");
        }

        if (currency.minTransferAmount <= 0) {
            throw ConfigException("币种 " + currencyId + " 的最小转账金额必须大于0");
        }

        if (currency.transferFee < 0) {
            throw ConfigException("币种 " + currencyId + " 的转账手续费不能为负数");
        }

        if (currency.feePercentage < 0 || currency.feePercentage > 100) {
            throw ConfigException("币种 " + currencyId + " 的转账手续费百分比必须在0-100之间");
        }
    }

    // 验证排行榜配置
    if (config.topList.defaultCount <= 0) {
        throw ConfigException("默认排行榜数量必须大于0");
    }
    if (config.topList.maxCount <= 0) {
        throw ConfigException("最大排行榜数量必须大于0");
    }
    if (config.topList.defaultCount > config.topList.maxCount) {
        throw ConfigException("默认排行榜数量不能大于最大数量");
    }
}

void ConfigManager::ensureConfigDirectory(const std::string& configPath) const {
    try {
        std::filesystem::path path(configPath);
        std::filesystem::path dir = path.parent_path();

        if (!dir.empty() && !std::filesystem::exists(dir)) {
            if (!std::filesystem::create_directories(dir)) {
                throw ConfigException("创建配置目录失败");
            }
        }

    } catch (const std::exception& e) {
        throw ConfigException("创建配置目录失败: " + std::string(e.what()));
    }
}

void ConfigManager::resetForTesting() {
    // 重置配置为默认状态
    mConfig = createDefaultConfig();
    mConfigPath.clear();
}

} // namespace rlx_money