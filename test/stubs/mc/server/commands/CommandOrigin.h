#pragma once

#ifdef TESTING

#include "../world/actor/Actor.h"

// Mock CommandOrigin for testing
class CommandOrigin {
private:
    Actor* entity_;

public:
    CommandOrigin() : entity_(nullptr) {}
    CommandOrigin(Actor* entity) : entity_(entity) {}
    
    Actor* getEntity() const { return entity_; }
};

#endif // TESTING

