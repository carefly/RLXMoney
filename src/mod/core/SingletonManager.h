#pragma once

namespace rlx_money {

/**
 * @brief 通用单例管理器模板
 *
 * 提供统一的单例访问和测试重置接口
 * 使用 C++11 线程安全的 Meyers' Singleton 模式
 *
 * @tparam T 单例类类型
 */
template <typename T>
class SingletonManager {
public:
    /**
     * @brief 获取单例实例
     *
     * 使用 C++11 线程安全的 Meyers' Singleton 模式
     * 首次访问时进行线程安全的初始化，后续访问无额外开销
     *
     * @return T 单例实例的引用
     */
    static T& getInstance() {
        static T instance;
        return instance;
    }

    /**
     * @brief 重置单例状态（仅用于测试）
     *
     * 如果单例类提供了 resetForTesting() 方法，则调用它
     * 使用 C++20 concepts 检查方法是否存在
     *
     * @note 此方法仅应在测试环境中使用
     */
    static void resetForTesting() {
        if constexpr (requires { T::resetForTesting(); }) {
            getInstance().resetForTesting();
        }
    }

private:
    // 禁用构造函数、拷贝构造和赋值操作
    SingletonManager()                                   = delete;
    ~SingletonManager()                                  = delete;
    SingletonManager(const SingletonManager&)            = delete;
    SingletonManager& operator=(const SingletonManager&) = delete;
};

} // namespace rlx_money