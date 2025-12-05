#pragma once

#ifdef TESTING

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include "MockLeviLaminaAPI.h"

// Mock ActorType
enum class ActorType { Player, Other };

// Mock Actor 基类
class Actor {
public:
    virtual ~Actor() = default;
    virtual bool isType(ActorType type) const = 0;
};

// Mock Player 类（使用 MockLeviLaminaAPI.h 中的 Player，但添加 Actor 接口）
class MockCommandPlayer : public Actor, public Player {
public:
    bool isOp = false;
    std::vector<std::string> sentMessages;

    MockCommandPlayer(const std::string& x, const std::string& n) : Player(x, n) {}

    bool isOperator() const { return isOp; }
    
    void sendMessage(const std::string& msg) {
        sentMessages.push_back(msg);
    }
    
    bool isType(ActorType type) const override {
        return type == ActorType::Player;
    }
};

// Mock CommandRawText
struct CommandRawText {
    std::string mText;
    CommandRawText() : mText("") {}
    CommandRawText(const std::string& text) : mText(text) {}
};

// Mock CommandOrigin
class CommandOrigin {
private:
    Actor* entity_;

public:
    CommandOrigin(Actor* entity = nullptr) : entity_(entity) {}
    
    Actor* getEntity() const { return entity_; }
};

// Mock CommandOutput
class CommandOutput {
private:
    std::vector<std::string> errors_;
    std::vector<std::string> successes_;

public:
    void error(const std::string& msg) {
        errors_.push_back(msg);
    }
    
    void success(const std::string& msg) {
        successes_.push_back(msg);
    }
    
    const std::vector<std::string>& getErrors() const { return errors_; }
    const std::vector<std::string>& getSuccesses() const { return successes_; }
    
    bool hasError() const { return !errors_.empty(); }
    bool hasSuccess() const { return !successes_.empty(); }
    
    std::string getLastError() const {
        return errors_.empty() ? "" : errors_.back();
    }
    
    std::string getLastSuccess() const {
        return successes_.empty() ? "" : successes_.back();
    }
    
    void clear() {
        errors_.clear();
        successes_.clear();
    }
};

// Mock Command
class Command {
public:
    // 空的 Mock 类
};

#endif // TESTING

