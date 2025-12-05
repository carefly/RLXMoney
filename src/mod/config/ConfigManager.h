#pragma once

#include "mod/config/ConfigStructures.h"
#include <string>


namespace rlx_money {

/// @brief 配置管理器类
class ConfigManager {
public:
    /// @brief 获取单例实例
    /// @return 配置管理器实例
    static ConfigManager& getInstance();

    /// @brief 加载配置文件
    /// @param configPath 配置文件路径
    void loadConfig(const std::string& configPath);

    /// @brief 重新加载配置文件
    void reloadConfig();

    /// @brief 获取配置
    /// @return 配置对象的常量引用
    [[nodiscard]] const ModConfig& getConfig() const;

    /// @brief 保存配置到文件
    void saveConfig() const;

    /// @brief 获取配置文件路径
    /// @return 配置文件路径
    [[nodiscard]] const std::string& getConfigPath() const;

    /// @brief 设置初始金额
    /// @param amount 初始金额
    void setInitialBalance(int amount);

    /// @brief 获取初始金额
    /// @return 初始金额
    [[nodiscard]] int getInitialBalance() const;

    /// @brief 设置玩家转账是否允许
    /// @param allow 是否允许
    void setAllowPlayerTransfer(bool allow);

    /// @brief 获取玩家转账是否允许
    /// @return 是否允许
    [[nodiscard]] bool getAllowPlayerTransfer() const;

    /// @brief 重置配置管理器状态（仅用于测试）
    /// @note 此方法仅用于测试，用于清理单例状态以便测试之间隔离
    void resetForTesting();

    ConfigManager(const ConfigManager&)            = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

private:
    ConfigManager()  = default;
    ~ConfigManager() = default;


    /// @brief 创建默认配置
    /// @return 默认配置对象
    ModConfig createDefaultConfig() const;

    /// @brief 验证配置有效性
    /// @param config 配置对象
    void validateConfig(const ModConfig& config) const;

    /// @brief 确保配置目录存在
    /// @param configPath 配置文件路径
    void ensureConfigDirectory(const std::string& configPath) const;

    ModConfig   mConfig;
    std::string mConfigPath;
};

} // namespace rlx_money