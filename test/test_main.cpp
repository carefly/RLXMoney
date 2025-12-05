#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>
#include <windows.h>
#include "utils/TestTempManager.h"

// 测试工程初始化设置
struct ConsoleInitializer {
    ConsoleInitializer() {
        SetConsoleOutputCP(CP_UTF8);   // 输出 UTF-8
        SetConsoleCP(CP_UTF8);         // 输入 UTF-8
    }
};

// 全局初始化对象，在程序启动时自动调用
static ConsoleInitializer consoleInit;

// 测试临时文件清理器，在程序结束时自动清理所有测试临时文件
struct TestTempCleanupGuard {
    ~TestTempCleanupGuard() {
        try {
            // 在程序结束时清理所有测试临时文件
            rlx_money::test::TestTempManager::getInstance().cleanup();
        } catch (...) {
            // 忽略清理时的异常，避免程序异常退出
        }
    }
};

// 全局清理对象，在程序结束时自动调用
static TestTempCleanupGuard tempCleanupGuard;

// 简单的数学函数测试示例
int add(int a, int b) { return a + b; }

int multiply(int a, int b) { return a * b; }

TEST_CASE("简单的数学运算测试", "[math]") {

    SECTION("加法测试") {
        REQUIRE(add(2, 3) == 5);
        REQUIRE(add(-1, 1) == 0);
        REQUIRE(add(0, 0) == 0);
        REQUIRE(add(-5, -3) == -8);
    }

    SECTION("乘法测试") {
        REQUIRE(multiply(2, 3) == 6);
        REQUIRE(multiply(-1, 1) == -1);
        REQUIRE(multiply(0, 5) == 0);
        REQUIRE(multiply(-2, -3) == 6);
    }
}