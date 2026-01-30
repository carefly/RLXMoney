#pragma once

#include <filesystem>
#include <format>
#include <fstream>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>


#ifdef _WIN32
#include <Windows.h>
#endif

namespace rlx::common {

/// @brief 通用配置管理器（头文件 Only，支持多 mod 共用）
class ConfigManager {
public:
    // ==================== Mod 节点管理 ====================

    /// @brief 设置当前 mod 的节点名称（并自动注册该节点）
    static void setModSection(const std::string& section) {
        getModSectionRef() = section;
        registerSection(section);
    }

    /// @brief 获取当前 mod 的节点名称
    static const std::string& getModSection() { return getModSectionRef(); }

    // ==================== 简化版 API（使用预设的 mod 节点） ====================

    /// @brief 获取配置项（布尔值）- 使用预设的 mod 节点
    static bool getBool(const std::string& key, bool defaultValue = false) {
        return getBool(getModSection(), key, defaultValue);
    }

    /// @brief 获取配置项（整数）- 使用预设的 mod 节点
    static int getInt(const std::string& key, int defaultValue = 0) {
        return getInt(getModSection(), key, defaultValue);
    }

    /// @brief 获取配置项（字符串）- 使用预设的 mod 节点
    static std::string getString(const std::string& key, const std::string& defaultValue = "") {
        return getString(getModSection(), key, defaultValue);
    }

    /// @brief 设置配置项（布尔值）- 使用预设的 mod 节点
    static void setBool(const std::string& key, bool value) { setBool(getModSection(), key, value); }

    /// @brief 设置配置项（整数）- 使用预设的 mod 节点
    static void setInt(const std::string& key, int value) { setInt(getModSection(), key, value); }

    /// @brief 设置配置项（字符串）- 使用预设的 mod 节点
    static void setString(const std::string& key, const std::string& value) { setString(getModSection(), key, value); }

    // ==================== 完整版 API（需要指定节点） ====================
    /// @brief 获取配置项（布尔值）
    static bool getBool(const std::string& section, const std::string& key, bool defaultValue = false) {
        ensureLoaded();
        auto& cached = getCache();
        if (cached.contains(section) && cached[section].contains(key)) {
            try {
                return cached[section][key].get<bool>();
            } catch (...) {}
        }
        return defaultValue;
    }

    /// @brief 获取配置项（整数）
    static int getInt(const std::string& section, const std::string& key, int defaultValue = 0) {
        ensureLoaded();
        auto& cached = getCache();
        if (cached.contains(section) && cached[section].contains(key)) {
            try {
                return cached[section][key].get<int>();
            } catch (...) {}
        }
        return defaultValue;
    }

    /// @brief 获取配置项（字符串）
    static std::string
    getString(const std::string& section, const std::string& key, const std::string& defaultValue = "") {
        ensureLoaded();
        auto& cached = getCache();
        if (cached.contains(section) && cached[section].contains(key)) {
            try {
                return cached[section][key].get<std::string>();
            } catch (...) {}
        }
        return defaultValue;
    }

    /// @brief 设置配置项（布尔值）
    static void setBool(const std::string& section, const std::string& key, bool value) {
        ensureLoaded();
        getCache()[section][key] = value;
        setDirty();
    }

    /// @brief 设置配置项（整数）
    static void setInt(const std::string& section, const std::string& key, int value) {
        ensureLoaded();
        getCache()[section][key] = value;
        setDirty();
    }

    /// @brief 设置配置项（字符串）
    static void setString(const std::string& section, const std::string& key, const std::string& value) {
        ensureLoaded();
        getCache()[section][key] = value;
        setDirty();
    }

    /// @brief 保存配置到文件
    static bool save(const std::string& configPath = getConfigPath()) {
        try {
            std::ofstream outFile(configPath);
            if (outFile.is_open()) {
                outFile << getCache().dump(4);
                outFile.close();
                clearDirty();
                return true;
            }
        } catch (...) {}
        return false;
    }

    /// @brief 设置日志回调（用于输出日志）
    using LogCallback = void (*)(const std::string& message);
    static void setLogCallback(LogCallback callback) { getLogCallbackPtr() = callback; }

    /// @brief 设置配置文件路径（需在首次访问前调用）
    static void setConfigPath(const std::string& path) { getConfigPathRef() = path; }

    /// @brief 注册配置节点（确保该节点在配置文件中存在）
    static void registerSection(const std::string& section) {
        ensureLoaded();
        if (!getCache().contains(section) || !getCache()[section].is_object()) {
            getCache()[section] = nlohmann::json::object();
            save();
        }
    }

    /// @brief 获取当前配置文件路径
    static const std::string& getConfigPath() { return getConfigPathRef(); }

    /// @brief 重置加载状态（强制下次访问时重新加载文件）
    static void resetLoaded() {
        isLoaded() = false;
    }

    /// @brief 检查 DLL 是否存在
    static bool checkDllExists(const std::string& dllName, const std::vector<std::string>& extraPaths = {}) {
        std::vector<std::string> searchPaths = {
            ".",
            "plugins",
            "../plugins",
        };
        // 添加额外搜索路径
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

private:
    static inline std::string& getModSectionRef() {
        static std::string section = "common";
        return section;
    }

    static inline nlohmann::json& getCache() {
        static nlohmann::json cache;
        return cache;
    }

    // ==================== Config<T> 友元访问 ====================
    template <typename U>
    friend class Config;

    static inline bool& isLoaded() {
        static bool loaded = false;
        return loaded;
    }

    static inline bool& isDirty() {
        static bool dirty = false;
        return dirty;
    }

    static inline std::string& getConfigPathRef() {
        static std::string path = "plugins/RLXModeResources/config/config.json";
        return path;
    }

    static inline LogCallback& getLogCallbackPtr() {
        static LogCallback callback = nullptr;
        return callback;
    }

    static void log(const std::string& message) {
        if (auto cb = getLogCallbackPtr()) {
            cb(message);
        }
    }

    static void setDirty() { isDirty() = true; }

    static void clearDirty() { isDirty() = false; }

    static inline void ensureLoaded() {
        if (!isLoaded()) {
            loadFromFile();
        }
    }

    static inline void loadFromFile() {
        const std::string& configPath = getConfigPathRef();

        // 确保配置目录存在
        std::filesystem::path configFile(configPath);
        std::filesystem::path configDir = configFile.parent_path();
        if (!configDir.empty()) {
            try {
                std::filesystem::create_directories(configDir);
            } catch (const std::exception& e) {
                log(std::format("Failed to create config directory: {}", e.what()));
            }
        }

        // 尝试加载配置文件
        std::ifstream file(configPath);
        bool          needWrite = false;

        if (file.is_open()) {
            try {
                file >> getCache();
                file.close();
            } catch (const std::exception&) {
                file.close();
                log(std::format("Failed to parse config file {}, creating default", configPath));
                getCache() = nlohmann::json::object();
                needWrite  = true;
            }
        } else {
            getCache() = nlohmann::json::object();
            needWrite  = true;
            log(std::format("Config file not found at {}, creating default", configPath));
        }

        // 如果需要写入，保存配置文件
        if (needWrite) {
            save();
        }

        isLoaded() = true;
    }
};

// ==================== 强类型配置模板 ====================

/// @brief 强类型配置包装器
/// @tparam T 配置结构体类型
///
/// 使用示例：
/// @code
/// // 1. 定义配置结构体（使用宏定义序列化）
/// struct MyConfig {
///     bool enable = true;
///     int maxCount = 100;
///     std::string name = "default";
/// };
/// NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MyConfig, enable, maxCount, name)
///
/// // 2. 声明配置实例
/// static Config<MyConfig> cfg("my_section");
///
/// // 3. 使用配置
/// if (cfg->enable) { ... }  // 读
/// cfg->maxCount = 200;      // 写
/// cfg.save();               // 保存（析构时也会自动保存）
/// @endcode
template <typename T>
class Config {
public:
    /// @brief 构造配置对象
    /// @param section 配置节点名称（在 JSON 中的顶层 key）
    explicit Config(const std::string& section) : section_(section), autoSave_(true) { load(); }

    /// @brief 析构时自动保存（如果启用）
    ~Config() {
        if (autoSave_) {
            save();
        }
    }

    // 禁止拷贝和移动
    Config(const Config&)                = delete;
    Config& operator=(const Config&)     = delete;
    Config(Config&&) noexcept            = delete;
    Config& operator=(Config&&) noexcept = delete;

    /// @brief 像指针一样访问配置成员
    T*       operator->() { return &data_; }
    const T* operator->() const { return &data_; }

    /// @brief 获取配置数据的引用
    T&       get() { return data_; }
    const T& get() const { return data_; }

    /// @brief 保存配置到文件
    void save() {
        try {
            ConfigManager::ensureLoaded();
            nlohmann::json j                    = data_;
            ConfigManager::getCache()[section_] = j;
            ConfigManager::save();
        } catch (...) {
            // 静默失败
        }
    }

    /// @brief 重新从文件加载配置
    void load() {
        try {
            auto& cache = ConfigManager::getCache();

            // 如果还没加载过文件，先用默认值初始化 cache
            // 这样 loadFromFile() 保存时就不是空 JSON，而是包含默认值的配置
            if (!ConfigManager::isLoaded()) {
                data_           = T{};
                nlohmann::json j = data_;
                cache[section_] = j;
            }

            ConfigManager::ensureLoaded();

            // 如果 JSON 中有该 section，读取并更新
            if (cache.contains(section_) && cache[section_].is_object()) {
                data_ = cache[section_].get<T>();
            } else {
                // section 不存在（文件被手动修改），使用默认值并写入文件
                data_           = T{};
                nlohmann::json j = data_;
                cache[section_] = j;
                ConfigManager::save();
            }
        } catch (...) {
            data_ = T{}; // 使用默认值
        }
    }

    /// @brief 启用/禁用自动保存
    void setAutoSave(bool enable) { autoSave_ = enable; }

    /// @brief 重置为默认值
    void reset() {
        data_ = T{};
        save();
    }

private:
    std::string section_;
    T           data_;
    bool        autoSave_;
};

} // namespace rlx::common
