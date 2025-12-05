#include "mod/events/PlayerEventListener.h"
#include "mod/dao/PlayerDAO.h"
#include "mod/database/DatabaseManager.h"
#include "mod/economy/EconomyManager.h"
#include "mod/exceptions/MoneyException.h"


#include <ll/api/event/EventBus.h>
#include <ll/api/event/player/PlayerJoinEvent.h>
#include <ll/api/mod/NativeMod.h>
#include <mc/server/ServerPlayer.h>
#include <mc/world/actor/player/Player.h>


namespace rlx_money {

static ll::event::ListenerPtr gPlayerJoinListener = nullptr;

void PlayerEventListener::registerListeners() {
    auto& eventBus = ll::event::EventBus::getInstance();
    auto& logger   = ll::mod::NativeMod::current()->getLogger();

    // 监听玩家加入事件
    gPlayerJoinListener =
        eventBus.emplaceListener<ll::event::player::PlayerJoinEvent>([](ll::event::player::PlayerJoinEvent& event) {
            auto& logger = ll::mod::NativeMod::current()->getLogger();
            try {
                auto&       player   = event.self();
                std::string xuid     = player.getXuid();
                std::string username = std::string(player.mName);

                // 检查玩家是否已存在
                if (!EconomyManager::getInstance().playerExists(xuid)) {
                    // 初始化新玩家账户
                    if (EconomyManager::getInstance().initializeNewPlayer(xuid, username)) {
                        logger.info("新玩家 {} ({}) 账户初始化成功", username, xuid);
                    } else {
                        logger.error("新玩家 {} ({}) 账户初始化失败", username, xuid);
                    }
                } else {
                    // 如果玩家已存在，更新用户名（以防用户名变更）
                    PlayerDAO playerDAO(DatabaseManager::getInstance());
                    auto      existingPlayer = playerDAO.getPlayerByXuid(xuid);
                    if (existingPlayer.has_value() && existingPlayer->username != username) {
                        playerDAO.updateUsername(xuid, username);
                        logger.debug("更新玩家 {} 的用户名为 {}", xuid, username);
                    }
                }
            } catch (const MoneyException& e) {
                // 如果玩家已存在，忽略异常（这是正常情况）
                if (e.getErrorCode() != ErrorCode::PLAYER_ALREADY_EXISTS) {
                    logger.error("处理玩家加入事件时发生异常: {}", e.what());
                }
            } catch (const std::exception& e) {
                // 记录错误但不阻止玩家加入
                logger.error("处理玩家加入事件时发生异常: {}", e.what());
            }
        });

    logger.info("玩家事件监听器已注册");
}

void PlayerEventListener::unregisterListeners() {
    if (gPlayerJoinListener) {
        auto& eventBus = ll::event::EventBus::getInstance();
        eventBus.removeListener(gPlayerJoinListener);
        gPlayerJoinListener = nullptr;
    }
}

} // namespace rlx_money
