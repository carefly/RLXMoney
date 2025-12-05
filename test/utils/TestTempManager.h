#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <chrono>

namespace rlx_money {
namespace test {

/**
 * 测试临时文件管理器
 * 负责创建和清理测试过程中产生的临时文件和目录
 */
class TestTempManager {
public:
    static TestTempManager& getInstance();

    /**
     * 获取测试临时目录路径
     * 如果目录不存在会自动创建
     */
    std::string getTempDir();

    /**
     * 生成唯一的临时文件路径
     * @param prefix 文件名前缀
     * @param extension 文件扩展名
     * @return 完整的临时文件路径
     */
    std::string makeUniquePath(const std::string& prefix, const std::string& extension);

    /**
     * 注册需要清理的文件路径
     * @param filePath 文件路径
     */
    void registerFile(const std::string& filePath);

    /**
     * 注册需要清理的目录路径
     * @param dirPath 目录路径
     */
    void registerDirectory(const std::string& dirPath);

    /**
     * 清理所有注册的文件和目录
     */
    void cleanup();

    /**
     * 清理所有测试临时文件（整个临时目录）
     */
    void cleanupAll();

    /**
     * 获取已注册的文件数量
     */
    size_t getRegisteredFileCount() const { return mRegisteredFiles.size(); }

    /**
     * 获取已注册的目录数量
     */
    size_t getRegisteredDirCount() const { return mRegisteredDirs.size(); }

private:
    TestTempManager() = default;
    ~TestTempManager();

    std::string mTempDir;
    std::vector<std::string> mRegisteredFiles;
    std::vector<std::string> mRegisteredDirs;

    void ensureTempDirExists();
};

/**
 * RAII风格的测试临时文件清理守护
 * 在作用域结束时自动清理注册的文件
 */
class TempFileGuard {
public:
    explicit TempFileGuard() = default;
    ~TempFileGuard();

    // 禁用复制和移动
    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
    TempFileGuard(TempFileGuard&&) = delete;
    TempFileGuard& operator=(TempFileGuard&&) = delete;

    /**
     * 注册需要清理的文件
     */
    void registerFile(const std::string& filePath);

    /**
     * 注册需要清理的目录
     */
    void registerDirectory(const std::string& dirPath);

    /**
     * 立即清理所有注册的文件
     */
    void cleanup();

private:
    std::vector<std::string> mFiles;
    std::vector<std::string> mDirs;
};

} // namespace test
} // namespace rlx_money