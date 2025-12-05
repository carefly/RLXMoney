#pragma once

#ifdef TESTING

// Mock CommandHandle for testing
// This is typically a template class, but for testing we just need a stub
template <typename T>
class CommandHandle {
public:
    // Empty mock class
};

#endif // TESTING
