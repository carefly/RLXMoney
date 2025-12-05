#pragma once

#include <mutex>

namespace rlx_money {

    // 前向声明，避免包含头文件
    class ConfigManager;
    class DatabaseManager;
    class EconomyManager;

    /**
     * @brief 系统初始化器
     *
     * 管理 RLXMoney 系统中所有单例的初始化顺序，确保依赖关系正确
     * 使用 std::call_once 保证初始化过程的线程安全和幂等性
     */
    class SystemInitializer {
    public:
        /**
         * @brief 初始化整个系统
         *
         * 按依赖顺序初始化所有单例：
         * 1. ConfigManager - 配置系统，无依赖
         * 2. DatabaseManager - 数据库系统，依赖配置
         * 3. EconomyManager - 经济系统，依赖配置和数据库
         *
         * 使用 std::call_once 确保只初始化一次，且线程安全
         */
        static void initialize();

        /**
         * @brief 重置所有单例（仅用于测试）
         *
         * 按与初始化相反的顺序重置所有单例状态
         * 确保测试环境的状态隔离
         *
         * @note 此方法仅应在测试环境中使用
         */
        static void resetAllForTesting();

    private:
        static std::once_flag initFlag;
    };

} // namespace rlx_money