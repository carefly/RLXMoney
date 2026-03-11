#pragma once

#include <filesystem>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace rlx::common {

// ==================== 类型特征检测 ====================

/// @brief 检测类型 T 是否有 validate() 方法
template <typename T, typename = void>
struct has_validate : std::false_type {};

template <typename T>
struct has_validate<T, std::void_t<decltype(std::declval<T>().validate())>> : std::true_type {};

template <typename T>
inline constexpr bool has_validate_v = has_validate<T>::value;

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
/// if (Config<MyConfig>::getInstance().get().enable) { ... }  // 读
/// Config<MyConfig>::getInstance().getWritable().maxCount = 200; // 写
/// Config<MyConfig>::getInstance().save();                     // 保存
///
/// // 4. 重置配置（测试用）
/// Config<MyConfig>::reset();
/// @endcode
///
/// 使用示例（独立实例）：
/// @code
/// Config<MyConfig> cfg("path/to/config.json");
/// if (cfg.get().enable) { ... }
/// cfg.getWritable().maxCount = 200;
/// cfg.save();
/// @endcode
template <typename T>
class Config {
public:
    // ==================== 单例 API ====================

    /// @brief 获取全局单例配置（必须先调用 init()）
    static Config& getInstance() {
        auto* instance = getInstancePtr().get();
        if (instance == nullptr) {
            throw std::runtime_error("Config not initialized. Call init() first.");
        }
        return *instance;
    }

    /// @brief 初始化全局单例配置
    /// @param configPath 配置文件路径
    /// @param autoLoad 是否自动加载（默认 true）
    static void init(std::string configPath, bool autoLoad = true) {
        if (getInstancePtr() != nullptr) {
            throw std::runtime_error("Config already initialized.");
        }
        getInstancePtr() = std::unique_ptr<Config>(new Config(std::move(configPath), true));
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
    static void reset() { getInstancePtr().reset(); }

    /// @brief 检查单例是否已初始化
    static bool isInitialized() { return getInstancePtr() != nullptr; }

    // ==================== 构造函数（用于独立实例） ====================

    /// @brief 构造独立配置实例（不使用全局单例）
    explicit Config(const std::string& configPath) : Config(configPath, false) { load(); }

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
        }
    }

    // 禁止拷贝和移动
    Config(const Config&)                = delete;
    Config& operator=(const Config&)     = delete;
    Config(Config&&) noexcept            = delete;
    Config& operator=(Config&&) noexcept = delete;

    // ==================== 数据访问 ====================

    /// @brief 像指针一样访问配置成员
    T* operator->() {
        markDirty();
        return &pImpl_->data;
    }

    const T* operator->() const { return &pImpl_->data; }

    /// @brief 获取配置数据的引用（只读）
    const T& get() const { return pImpl_->data; }

    /// @brief 获取配置数据的引用（可写，会标记 dirty）
    T& getWritable() {
        markDirty();
        return pImpl_->data;
    }

    // ==================== 配置操作 ====================

    /// @brief 保存配置到文件（如果 T 有 validate() 方法，保存前会自动调用）
    void save() {
        if constexpr (has_validate_v<T>) {
            pImpl_->data.validate();
        }
        saveFile();
        pImpl_->dirty = false;
    }

    /// @brief 重新从文件加载配置（如果 T 有 validate() 方法，加载后会自动调用）
    void load() {
        loadFile();
        if constexpr (has_validate_v<T>) {
            pImpl_->data.validate();
        }
        pImpl_->dirty = false;
    }

    /// @brief 重新加载配置（load 的别名）
    void reload() { load(); }

    /// @brief 重置为默认值并保存
    void resetToDefault() {
        pImpl_->data = T{};
        save();
    }

    /// @brief 检查配置文件是否存在
    [[nodiscard]] bool fileExists() const { return std::filesystem::exists(pImpl_->configPath); }

    /// @brief 获取配置文件路径
    [[nodiscard]] const std::string& getPath() const { return pImpl_->configPath; }

    // ==================== 自动保存控制 ====================

    /// @brief 启用/禁用自动保存（析构时）
    void setAutoSave(bool enable) { pImpl_->autoSave = enable; }

    /// @brief 获取配置是否已修改（脏标记）
    [[nodiscard]] bool isDirty() const { return pImpl_->dirty; }

private:
    /// @brief 内部实现结构（Pimpl 模式）
    struct Impl {
        std::string configPath;
        T           data{};
        bool        autoSave    = true;
        bool        dirty       = false;

        explicit Impl(const std::string& path) : configPath(path) {}
    };

    std::unique_ptr<Impl> pImpl_;

    /// @brief 私有构造函数（区分单例和独立实例）
    Config(const std::string& configPath, bool /*isGlobalInstance*/)
        : pImpl_(new Impl(configPath)) {}

    /// @brief 获取单例指针的静态引用
    static std::unique_ptr<Config>& getInstancePtr() {
        static std::unique_ptr<Config> instance;
        return instance;
    }

    /// @brief 标记为已修改
    void markDirty() { pImpl_->dirty = true; }

    /// @brief 确保配置目录存在
    void ensureDirectoryExists() const {
        std::filesystem::path configFile(pImpl_->configPath);
        std::filesystem::path configDir = configFile.parent_path();
        if (!configDir.empty()) {
            std::filesystem::create_directories(configDir);
        }
    }

    /// @brief 保存配置到文件（原子写入）
    void saveFile() {
        ensureDirectoryExists();

        // 序列化配置数据
        nlohmann::json j = pImpl_->data;

        // 原子写入：先写临时文件，再重命名
        std::string   tempPath = std::string(pImpl_->configPath) + ".tmp";
        std::ofstream outFile(tempPath);
        if (!outFile.is_open()) {
            throw std::runtime_error("Failed to open config file for writing: " + tempPath);
        }
        outFile << j.dump(4);
        if (!outFile.good()) {
            outFile.close();
            throw std::runtime_error("Failed to write config data to: " + tempPath);
        }
        outFile.close();

        // 重命名是原子操作（同一文件系统内）
        try {
            std::filesystem::rename(tempPath, pImpl_->configPath);
        } catch (const std::exception& e) {
            throw std::runtime_error("Failed to rename temp file to config: " + std::string(e.what()));
        }
    }

    /// @brief 从文件加载配置
    void loadFile() {
        ensureDirectoryExists();

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
