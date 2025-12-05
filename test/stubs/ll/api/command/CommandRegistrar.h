#pragma once

#ifdef TESTING

#include <string>

// Mock CommandPermissionLevel
enum class CommandPermissionLevel { Any = 0, GameDirectors = 1, Admin = 2, Host = 3, Owner = 4, Internal = 5 };

// Mock Command class
class Command {
public:
    // Empty mock class
};

// Mock CommandRegistrar for testing
class CommandRegistrar {
private:
    static CommandRegistrar* instance_;

public:
    static CommandRegistrar& getInstance() {
        if (!instance_) {
            instance_ = new CommandRegistrar();
        }
        return *instance_;
    }

    template <typename T>
    class CommandBuilder {
    public:
        CommandBuilder& required(const std::string& name) { return *this; }
        CommandBuilder& optional(const std::string& name) { return *this; }

        template <typename Func>
        CommandBuilder& execute(Func&& func) {
            return *this;
        }
    };

    template <typename T>
    CommandBuilder<T> overload() {
        return CommandBuilder<T>();
    }

    Command& getOrCreateCommand(const std::string& name, const std::string& description) {
        static Command cmd;
        return cmd;
    }

    Command& getOrCreateCommand(const std::string& name, const std::string& description, CommandPermissionLevel level) {
        static Command cmd;
        return cmd;
    }
};

// Static member definition
CommandRegistrar* CommandRegistrar::instance_ = nullptr;

#endif // TESTING
