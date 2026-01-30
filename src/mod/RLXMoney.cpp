#include "mod/RLXMoney.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mod/commands/Commands.h"
#include "mod/config/MoneyConfig.h"
#include "mod/core/SystemInitializer.h"
#include "mod/database/DatabaseManager.h"
#include "mod/economy/EconomyManager.h"
#include "mod/events/PlayerEventListener.h"
#include <exception>


namespace rlx_money {

RLXMoney& RLXMoney::getInstance() {
    static RLXMoney instance;
    return instance;
}

bool RLXMoney::load() {
    auto& logger = getSelf().getLogger();
    logger.info("正在加载 RLXMoney 插件...");

    try {
        // 初始化所有组件
        if (!initializeComponents()) {
            logger.error("初始化组件失败");
            return false;
        }

        logger.info("RLXMoney 插件加载完成");
        return true;

    } catch (const std::exception& e) {
        logger.error("加载插件时发生异常: {}", e.what());
        return false;
    }
}

bool RLXMoney::enable() {
    auto& logger = getSelf().getLogger();
    logger.info("正在启用 RLXMoney 插件...");

    try {
        // 使用 SystemInitializer 初始化所有单例
        logger.info("正在初始化系统组件...");
        SystemInitializer::initialize();

        // 初始化命令系统
        Commands::registerCommands();

        // 注册事件监听器
        PlayerEventListener::registerListeners();

        logger.info("RLXMoney 插件启用完成");
        mInitialized = true;
        return true;

    } catch (const std::exception& e) {
        logger.error("启用插件时发生异常: {}", e.what());
        return false;
    }
}

bool RLXMoney::disable() {
    auto& logger = getSelf().getLogger();
    logger.info("正在禁用 RLXMoney 插件...");

    try {
        // 取消注册事件监听器
        PlayerEventListener::unregisterListeners();

        // 清理所有组件
        cleanupComponents();

        logger.info("RLXMoney 插件禁用完成");
        mInitialized = false;
        return true;

    } catch (const std::exception& e) {
        logger.error("禁用插件时发生异常: {}", e.what());
        return false;
    }
}

bool RLXMoney::initializeComponents() const {
    auto& logger = getSelf().getLogger();

    try {
        // 初始化配置管理器
        logger.info("初始化配置管理器...");
        MoneyConfig::initialize();

        // 初始化数据库管理器
        logger.info("初始化数据库管理器...");
        const auto& config = MoneyConfig::get();
        if (!DatabaseManager::getInstance().initialize(config.database.path)) {
            logger.error("数据库初始化失败");
            return false;
        }

        // 初始化经济管理器
        logger.info("初始化经济管理器...");
        if (!EconomyManager::getInstance().initialize()) {
            logger.error("经济管理器初始化失败");
            return false;
        }

        logger.info("所有组件初始化完成");
        return true;

    } catch (const std::exception& e) {
        logger.error("初始化组件时发生异常: {}", e.what());
        return false;
    }
}

void RLXMoney::cleanupComponents() const {
    auto& logger = getSelf().getLogger();

    // try {
    //     logger.info("正在清理组件...");

    //     // 清理命令系统
    //     // Commands::cleanupCommands();

    //     // 关闭数据库连接
    //     DatabaseManager::getInstance().close();

    //     logger.info("组件清理完成");

    // } catch (const std::exception& e) {
    //     logger.error("清理组件时发生异常: {}", e.what());
    // }
    logger.info("该插件暂不支持卸载");
}

} // namespace rlx_money

LL_REGISTER_MOD(rlx_money::RLXMoney, rlx_money::RLXMoney::getInstance());