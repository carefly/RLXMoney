#pragma once

#ifdef TESTING

#include <string>
#include <vector>

// Mock CommandOutput for testing
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

#endif // TESTING



