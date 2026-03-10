#pragma once

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace rlx::common {

// ==================== 工具函数 ====================

/// @brief 检查 DLL 是否存在
inline bool checkDllExists(const std::string& dllName, const std::vector<std::string>& extraPaths = {}) {
    std::vector<std::string> searchPaths = {
        ".",
        "plugins",
        "../plugins",
    };
    searchPaths.insert(searchPaths.end(), extraPaths.begin(), extraPaths.end());

    for (const auto& basePath : searchPaths) {
        std::filesystem::path dllPath = std::filesystem::path(basePath) / dllName;
        if (std::filesystem::exists(dllPath) && std::filesystem::is_regular_file(dllPath)) {
            return true;
        }
    }

#ifdef _WIN32
    HMODULE hModule = LoadLibraryA(dllName.c_str());
    if (hModule != nullptr) {
        FreeLibrary(hModule);
        return true;
    }
#endif

    return false;
}

// ==================== 强类型配置模板 ====================

/// @brief 强类型配置包装器（支持单例模式和独立实例）
/// @tparam T 配置结构体类型
///
/// 使用示例（单例模式）：
/// @code
/// // 1. 定义配置结构体
/// struct MyConfig {
///     bool enable = true;
///     int maxCount = 100;
///     std::string name = "default";
/// };
/// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MyConfig, enable, maxCount, name)
///
/// // 2. 初始化全局配置（在插件启动时调用一次）
/// // 方式1: 使用固定路径前缀（推荐）
/// Config<MyConfig>::initWithName("my_mod.json");  // 路径: plugins/RLXModeResources/config/my_mod.json
///
/// // 方式2: 使用完整路径（用于测试或自定义路径）
/// Config<MyConfig>::init("custom/path/to/config.json");
///
/// // 3. 使用配置
/// if (Config<MyConfig>::getInstance()->enable) { ... }  // 读
/// Config<MyConfig>::getInstance()->maxCount = 200;      // 写
/// Config<MyConfig>::getInstance().save();               // 保存
///
/// // 4. 重置配置（测试用）
/// Config<MyConfig>::reset();
/// @endcode
///
/// 使用示例（独立实例）：
/// @code
/// Config<MyConfig> cfg("path/to/config.json");
/// if (cfg->enable) { ... }
/// cfg.save();
/// @endcode
template <typename T>
class Config {
public:
    // ==================== 单例 API ====================

    /// @brief 获取全局单例配置（必须先调用 init()）
    static Config& getInstance() {
        Config* instance = getInstancePtr();
        if (instance == nullptr) {
            throw std::runtime_error("Config not initialized. Call init() first.");
        }
        return *instance;
    }

    /// @brief 初始化全局单例配置
    /// @param configPath 配置文件路径
    /// @param autoLoad 是否自动加载（默认 true）
    static void init(const std::string& configPath, bool autoLoad = true) {
        if (getInstancePtr() != nullptr) {
            throw std::runtime_error("Config already initialized.");
        }
        getInstancePtr() = new Config(configPath, true);
        if (autoLoad) {
            getInstancePtr()->load();
        }
    }

    /// @brief 使用固定路径前缀初始化全局单例配置
    /// @param fileName 配置文件名（仅文件名，如 "rlx_money.json"）
    /// @param autoLoad 是否自动加载（默认 true）
    ///
    /// 配置文件将自动放置在固定路径：plugins/RLXModeResources/config/{fileName}
    static void initWithName(const std::string& fileName, bool autoLoad = true) {
        const std::string fixedPrefix = "plugins/RLXModeResources/config/";
        init(fixedPrefix + fileName, autoLoad);
    }

    /// @brief 重置全局单例（用于测试）
    static void reset() {
        Config* instance = getInstancePtr();
        if (instance != nullptr) {
            delete instance;
            getInstancePtr() = nullptr;
        }
    }

    /// @brief 检查单例是否已初始化
    static bool isInitialized() {
        return getInstancePtr() != nullptr;
    }

    // ==================== 构造函数（用于独立实例） ====================

    /// @brief 构造独立配置实例（不使用全局单例）
    explicit Config(const std::string& configPath)
        : Config(configPath, false) {
        load();
    }

    /// @brief 析构时自动保存（如果启用）
    ~Config() {
        if (pImpl_) {
            if (pImpl_->autoSave && pImpl_->dirty) {
                try {
                    saveFile();
                } catch (...) {
                    // 静默失败
                }
            }
            // 仅当不是全局单例时才删除 pImpl
            if (!pImpl_->isGlobalInstance) {
                delete pImpl_;
            } else {
                // 全局单例由 reset() 负责删除
                pImpl_ = nullptr;
            }
        }
    }

    // 禁止拷贝和移动
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;
    Config(Config&&) noexcept = delete;
    Config& operator=(Config&&) noexcept = delete;

    // ==================== 数据访问 ====================

    /// @brief 像指针一样访问配置成员
    T* operator->() {
        markDirty();
        return &pImpl_->data;
    }

    const T* operator->() const {
        return &pImpl_->data;
    }

    /// @brief 获取配置数据的引用
    T& get() {
        markDirty();
        return pImpl_->data;
    }

    const T& get() const {
        return pImpl_->data;
    }

    // ==================== 配置操作 ====================

    /// @brief 保存配置到文件
    void save() {
        saveFile();
        pImpl_->dirty = false;
    }

    /// @brief 重新从文件加载配置
    void load() {
        loadFile();
        pImpl_->dirty = false;
    }

    /// @brief 重新加载配置（load 的别名）
    void reload() {
        load();
    }

    /// @brief 重置为默认值并保存
    void resetToDefault() {
        pImpl_->data = T{};
        save();
    }

    /// @brief 检查配置文件是否存在
    [[nodiscard]] bool fileExists() const {
        return std::filesystem::exists(pImpl_->configPath);
    }

    /// @brief 获取配置文件路径
    [[nodiscard]] const std::string& getPath() const {
        return pImpl_->configPath;
    }

    // ==================== 自动保存控制 ====================

    /// @brief 启用/禁用自动保存（析构时）
    void setAutoSave(bool enable) {
        pImpl_->autoSave = enable;
    }

    /// @brief 获取配置是否已修改（脏标记）
    [[nodiscard]] bool isDirty() const {
        return pImpl_->dirty;
    }

private:
    /// @brief 内部实现结构（Pimpl 模式）
    struct Impl {
        std::string configPath;
        T           data;
        bool        autoSave;
        bool        dirty;
        bool        isGlobalInstance;

        Impl(const std::string& path, bool isGlobal)
            : configPath(path)
            , data(T{})
            , autoSave(true)
            , dirty(false)
            , isGlobalInstance(isGlobal) {}
    };

    Impl* pImpl_;

    /// @brief 私有构造函数（区分单例和独立实例）
    Config(const std::string& configPath, bool isGlobalInstance)
        : pImpl_(new Impl(configPath, isGlobalInstance)) {}

    /// @brief 获取单例指针的静态引用
    static Config*& getInstancePtr() {
        static Config* instance = nullptr;
        return instance;
    }

    /// @brief 标记为已修改
    void markDirty() {
        pImpl_->dirty = true;
    }

    /// @brief 保存配置到文件（原子写入）
    void saveFile() {
        // 确保配置目录存在
        std::filesystem::path configFile(pImpl_->configPath);
        std::filesystem::path configDir = configFile.parent_path();
        if (!configDir.empty()) {
            std::filesystem::create_directories(configDir);
        }

        // 序列化配置数据
        nlohmann::json j = pImpl_->data;

        // 原子写入：先写临时文件，再重命名
        std::string tempPath = std::string(pImpl_->configPath) + ".tmp";
        std::ofstream outFile(tempPath);
        if (outFile.is_open()) {
            outFile << j.dump(4);
            outFile.close();

            // 重命名是原子操作（同一文件系统内）
            std::filesystem::rename(tempPath, pImpl_->configPath);
        }
    }

    /// @brief 从文件加载配置
    void loadFile() {
        // 确保配置目录存在
        std::filesystem::path configFile(pImpl_->configPath);
        std::filesystem::path configDir = configFile.parent_path();
        if (!configDir.empty()) {
            std::filesystem::create_directories(configDir);
        }

        // 尝试加载配置文件
        std::ifstream file(pImpl_->configPath);

        if (file.is_open()) {
            try {
                nlohmann::json j;
                file >> j;
                file.close();
                pImpl_->data = j.get<T>();
            } catch (const std::exception&) {
                file.close();
                pImpl_->data = T{};
                save();
            }
        } else {
            pImpl_->data = T{};
            save();
        }
    }
};

} // namespace rlx::common
