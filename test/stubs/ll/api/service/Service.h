#pragma once

#ifdef TESTING

// Mock Service for testing
// This is typically a template class, but for testing we just need a stub
namespace ll::service {
    template<typename T>
    class Service {
    public:
        // Empty mock class
    };
}

#endif // TESTING



