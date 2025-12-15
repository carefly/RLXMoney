#include "mod/MyMoneyPlugin.h"
#include "mod/api/RLXMoneyAPI.h"
#include "ll/api/mod/RegisterHelper.h"
#include "ll/api/service/Bedrock.h"
#include "mc/world/actor/player/Player.h"
#include "ll/api/event/EventBus.h"
#include "ll/api/event/player/PlayerJoinEvent.h"
#include "ll/api/event/player/PlayerChatEvent.h"

#include <iostream>

namespace my_money_plugin {

MyMoneyPlugin& MyMoneyPlugin::getInstance() {
    static MyMoneyPlugin instance;
    return instance;
}

bool MyMoneyPlugin::load() {
    auto& logger = getSelf().getLogger();
    logger.info("正在加载 Money Plugin...");

    // 检查 RLXMoney 是否可用
    if (!rlx_money::RLXMoneyAPI::isInitialized()) {
        logger.warn("RLXMoney API 尚未初始化，某些功能可能不可用");
    }

    logger.info("Money Plugin 加载完成");
    return true;
}

bool MyMoneyPlugin::enable() {
    auto& logger = getSelf().getLogger();
    logger.info("正在启用 Money Plugin...");

    // 注册事件监听
    auto& eventBus = ll::event::EventBus::getInstance();

    // 监听玩家加入事件
    eventBus.addListener<ll::event::PlayerJoinEvent>([this](ll::event::PlayerJoinEvent& event) {
        onPlayerJoin(event);
    });

    // 监听玩家聊天事件（展示余额查询）
    eventBus.addListener<ll::event::PlayerChatEvent>([this](ll::event::PlayerChatEvent& event) {
        onPlayerChat(event);
    });

    logger.info("Money Plugin 启用完成");
    return true;
}

bool MyMoneyPlugin::disable() {
    auto& logger = getSelf().getLogger();
    logger.info("正在禁用 Money Plugin...");

    // 清理资源
    // （本示例中没有需要特殊清理的资源）

    logger.info("Money Plugin 禁用完成");
    return true;
}

void MyMoneyPlugin::onPlayerJoin(ll::event::PlayerJoinEvent& event) {
    auto& logger = getSelf().getLogger();
    auto* player = event.self();

    if (!player) {
        return;
    }

    std::string xuid = player->getXuid();
    std::string playerName = player->getName();

    logger.info("玩家 {} ({}) 加入了服务器", playerName, xuid);

    // 检查玩家是否已经存在经济数据
    if (!rlx_money::RLXMoneyAPI::playerExists(xuid)) {
        logger.info("新玩家 {}，正在初始化经济数据...", playerName);

        // 获取默认币种
        std::string defaultCurrency = rlx_money::RLXMoneyAPI::getDefaultCurrencyId();

        // 设置初始余额
        bool success = rlx_money::RLXMoneyAPI::addMoney(
            xuid,
            defaultCurrency,
            1000,
            "新玩家初始金额"
        );

        if (success) {
            auto balance = rlx_money::RLXMoneyAPI::getBalance(xuid, defaultCurrency);
            if (balance.has_value()) {
                logger.info("成功为玩家 {} 初始化 {} 币种余额: {}",
                           playerName, defaultCurrency, balance.value());
            }
        } else {
            logger.error("初始化玩家 {} 的经济数据失败", playerName);
        }
    } else {
        // 玩家已存在，显示当前余额
        auto balances = rlx_money::RLXMoneyAPI::getAllBalances(xuid);
        if (!balances.empty()) {
            logger.info("玩家 {} 的余额:", playerName);
            for (const auto& balance : balances) {
                logger.info("  {}: {}", balance.currencyId, balance.balance);
            }
        }
    }
}

void MyMoneyPlugin::onPlayerChat(ll::event::PlayerChatEvent& event) {
    auto* player = event.self();
    if (!player) {
        return;
    }

    std::string message = event.getMessage();

    // 简单的余额查询命令示例
    if (message == "?余额" || message == "?balance") {
        event.setCancelled(true); // 取消原聊天消息

        std::string xuid = player->getXuid();
        auto balances = rlx_money::RLXMoneyAPI::getAllBalances(xuid);

        if (balances.empty()) {
            player->sendMessage("§c你没有任何余额数据");
        } else {
            player->sendMessage("§a你的余额:");
            for (const auto& balance : balances) {
                // 这里应该使用币种的显示格式，但为简化示例，我们直接显示
                player->sendMessage(fmt::format("§e{}: §f{}", balance.currencyId, balance.balance));
            }
        }
    }

    // 转账示例命令
    if (message.find("?转账 ") == 0 || message.find("?transfer ") == 0) {
        event.setCancelled(true);

        // 解析命令: ?转账 <玩家名> <金额> [币种]
        // 这里简化处理，实际应该使用更完善的命令解析
        // 为演示目的，这里只做简单示例

        player->sendMessage("§c转账功能示例 - 请使用正式的钱币命令 /money pay");
    }
}

} // namespace my_money_plugin

// 注册插件
LL_REGISTER_MOD(my_money_plugin::MyMoneyPlugin, my_money_plugin::MyMoneyPlugin::getInstance());