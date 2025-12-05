#ifdef TESTING

#include "MockLeviLaminaAPI.h"

namespace rlx_money {

// 静态成员定义
std::unordered_map<std::string, std::shared_ptr<Player>> LeviLaminaAPI::xuidToPlayer;
std::unordered_map<std::string, std::shared_ptr<Player>> LeviLaminaAPI::nameToPlayer;

void LeviLaminaAPI::addMockPlayer(const std::string& xuid, const std::string& name) {
    auto player        = std::make_shared<Player>(xuid, name);
    xuidToPlayer[xuid] = player;
    nameToPlayer[name] = player;
}

void LeviLaminaAPI::clearMockPlayers() {
    xuidToPlayer.clear();
    nameToPlayer.clear();
}

Player* LeviLaminaAPI::getPlayerByXuid(const std::string& xuid) {
    auto it = xuidToPlayer.find(xuid);
    if (it != xuidToPlayer.end()) {
        return it->second.get();
    }
    return nullptr;
}

Player* LeviLaminaAPI::getPlayerByName(const std::string& name) {
    auto it = nameToPlayer.find(name);
    if (it != nameToPlayer.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::string LeviLaminaAPI::getPlayerNameByXuid(const std::string& xuid) {
    Player* player = getPlayerByXuid(xuid);
    if (player) {
        return player->name;
    }
    return {};
}

std::string LeviLaminaAPI::getXuidByPlayerName(const std::string& name) {
    Player* player = getPlayerByName(name);
    if (player) {
        return player->xuid;
    }
    return {};
}

} // namespace rlx_money

#endif // TESTING