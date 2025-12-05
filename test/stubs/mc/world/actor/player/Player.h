#pragma once

#ifdef TESTING

#include "../Actor.h"
#include "../../../mocks/MockLeviLaminaAPI.h"
#include <string>
#include <vector>

// Mock Player class that extends Actor and uses MockLeviLaminaAPI::Player
class Player : public Actor, public ::Player {
public:
    bool isOp = false;
    std::vector<std::string> sentMessages;

    Player(const std::string& x, const std::string& n) : ::Player(x, n) {}

    bool isOperator() const { return isOp; }
    
    void sendMessage(const std::string& msg) {
        sentMessages.push_back(msg);
    }
    
    bool isType(ActorType type) const override {
        return type == ActorType::Player;
    }
};

#endif // TESTING

