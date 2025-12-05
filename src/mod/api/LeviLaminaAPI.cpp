#include "mod/api/LeviLaminaAPI.h"
#include "ll/api/service/Bedrock.h"
#include "mc/world/actor/player/Player.h"
#include "mc/world/level/Level.h"

#include <algorithm>

namespace rlx_money {

Player* LeviLaminaAPI::getPlayerByXuid(const std::string& xuid) {
    Player* foundPlayer = nullptr;
    ll::service::getLevel()->forEachPlayer([&](Player& player) {
        if (player.getXuid() == xuid) {
            foundPlayer = &player;
            return false;
        }
        return true;
    });

    return foundPlayer;
}

Player* LeviLaminaAPI::getPlayerByName(const std::string& name) {
    Player* foundPlayer = nullptr;

    // 创建输入名称的小写副本用于比较
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

    ll::service::getLevel()->forEachPlayer([&](Player& player) {
        std::string pName = player.mName;
        std::transform(pName.begin(), pName.end(), pName.begin(), ::tolower);
        if (pName == lowerName) {
            foundPlayer = &player;
            return false;
        }
        return true;
    });

    return foundPlayer;
}

std::string LeviLaminaAPI::getPlayerNameByXuid(const std::string& xuid) {
    Player* foundPlayer = getPlayerByXuid(xuid);
    if (foundPlayer) {
        return std::string(foundPlayer->mName);
    }
    return {};
}

std::string LeviLaminaAPI::getXuidByPlayerName(const std::string& name) {
    Player* foundPlayer = getPlayerByName(name);
    if (foundPlayer) {
        return std::string(foundPlayer->getXuid());
    }
    return {};
}

} // namespace rlx_money