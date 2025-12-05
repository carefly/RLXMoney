#pragma once

#ifdef TESTING

// Mock ActorType
enum class ActorType { 
    Player, 
    Other 
};

// Mock Actor base class
class Actor {
public:
    virtual ~Actor() = default;
    virtual bool isType(ActorType type) const = 0;
};

#endif // TESTING



