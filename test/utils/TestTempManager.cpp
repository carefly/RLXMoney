#include "TestTempManager.h"
#include <iostream>


namespace rlx_money::test {

TestTempManager& TestTempManager::getInstance() {
    static TestTempManager instance;
    return instance;
}

TestTempManager::~TestTempManager() { cleanup(); }

std::string TestTempManager::getTempDir() {
    if (mTempDir.empty()) {
        // 使用项目根目录下的 test_temp 目录
        mTempDir = std::filesystem::absolute("test_temp").string();
        ensureTempDirExists();
    }
    return mTempDir;
}

void TestTempManager::ensureTempDirExists() {
    if (!std::filesystem::exists(mTempDir)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(mTempDir, ec)) {
            std::cerr << "警告: 无法创建测试临时目录 " << mTempDir << ": " << ec.message() << std::endl;
        }
    }
}

std::string TestTempManager::makeUniquePath(const std::string& prefix, const std::string& extension) {
    auto        now       = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::string filename  = prefix;
    filename             += "_";
    filename             += std::to_string(now);
    filename             += extension;

    // 确保文件名唯一
    auto fullPath = std::filesystem::path(getTempDir()) / filename;
    int  counter  = 1;
    while (std::filesystem::exists(fullPath)) {
        filename  = prefix;
        filename += "_";
        filename += std::to_string(now);
        filename += "_";
        filename += std::to_string(counter);
        filename += extension;
        fullPath  = std::filesystem::path(getTempDir()) / filename;
        counter++;
    }

    return fullPath.string();
}

void TestTempManager::registerFile(const std::string& filePath) {
    mRegisteredFiles.push_back(filePath);

    // 如果是数据库文件，自动注册相关的WAL和SHM文件
    if (filePath.ends_with(".db")) {
        std::string walPath  = filePath;
        walPath             += "-wal";
        mRegisteredFiles.push_back(walPath);

        std::string shmPath  = filePath;
        shmPath             += "-shm";
        mRegisteredFiles.push_back(shmPath);
    }
}

void TestTempManager::registerDirectory(const std::string& dirPath) { mRegisteredDirs.push_back(dirPath); }

void TestTempManager::cleanup() {
    // 清理注册的文件
    for (const auto& file : mRegisteredFiles) {
        try {
            if (std::filesystem::exists(file)) {
                std::filesystem::remove(file);
            }
        } catch (const std::exception& e) {
            std::cerr << "警告: 无法删除文件 " << file << ": " << e.what() << std::endl;
        }
    }
    mRegisteredFiles.clear();

    // 清理注册的目录
    for (const auto& dir : mRegisteredDirs) {
        try {
            if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
                std::filesystem::remove_all(dir);
            }
        } catch (const std::exception& e) {
            std::cerr << "警告: 无法删除目录 " << dir << ": " << e.what() << std::endl;
        }
    }
    mRegisteredDirs.clear();
}

void TestTempManager::cleanupAll() {
    try {
        std::string tempDir = getTempDir();
        if (std::filesystem::exists(tempDir)) {
            std::filesystem::remove_all(tempDir);
            std::cout << "已清理测试临时目录: " << tempDir << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "警告: 无法清理测试临时目录: " << e.what() << std::endl;
    }

    mTempDir.clear();
    mRegisteredFiles.clear();
    mRegisteredDirs.clear();
}

// TempFileGuard 实现
TempFileGuard::~TempFileGuard() { cleanup(); }

void TempFileGuard::registerFile(const std::string& filePath) { mFiles.push_back(filePath); }

void TempFileGuard::registerDirectory(const std::string& dirPath) { mDirs.push_back(dirPath); }

void TempFileGuard::cleanup() {
    // 清理文件
    for (const auto& file : mFiles) {
        try {
            if (std::filesystem::exists(file)) {
                std::filesystem::remove(file);
            }
        } catch (const std::exception& e) {
            std::cerr << "警告: 无法删除文件 " << file << ": " << e.what() << std::endl;
        }
    }
    mFiles.clear();

    // 清理目录
    for (const auto& dir : mDirs) {
        try {
            if (std::filesystem::exists(dir) && std::filesystem::is_directory(dir)) {
                std::filesystem::remove_all(dir);
            }
        } catch (const std::exception& e) {
            std::cerr << "警告: 无法删除目录 " << dir << ": " << e.what() << std::endl;
        }
    }
    mDirs.clear();
}

} // namespace rlx_money::test