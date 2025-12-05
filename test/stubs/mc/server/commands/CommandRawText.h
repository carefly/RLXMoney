#pragma once

#ifdef TESTING

#include <string>

// Mock CommandRawText for testing
struct CommandRawText {
    std::string mText;

    CommandRawText() : mText("") {}
    CommandRawText(const std::string& text) : mText(text) {}
    CommandRawText(const char* text) : mText(text ? text : "") {}

    bool empty() const { return mText.empty(); }
    operator std::string() const { return mText; }
};

#endif // TESTING
