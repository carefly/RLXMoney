#include "SystemInitializer.h"
#include "../config/MoneyConfig.h"
#include "../database/DatabaseManager.h"
#include "../economy/EconomyManager.h"

namespace rlx_money {

std::once_flag SystemInitializer::initFlag;

void SystemInitializer::initialize() {
    std::call_once(initFlag, []() {
        try {
            // 按依赖顺序初始化所有单例
            // 1. 配置系统 - 已在 RLXMoney::load() 中初始化
            // 2. 数据库系统 - 依赖配置
            DatabaseManager::getInstance();

            // 3. 经济系统 - 依赖配置和数据库
            EconomyManager::getInstance();

        } catch (const std::exception&) {
            // 重新抛出异常，让调用者处理日志记录
            throw;
        }
    });
}

void SystemInitializer::resetAllForTesting() {
    try {
        // 按相反顺序重置，避免依赖冲突
        EconomyManager::getInstance().resetForTesting();
        DatabaseManager::getInstance().resetForTesting();
        MoneyConfig::resetForTesting();

    } catch (const std::exception&) {
        // 重新抛出异常，让调用者处理日志记录
        throw;
    }
}

} // namespace rlx_money