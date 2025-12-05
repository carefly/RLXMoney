#pragma once

#include "mod/types/Types.h"
#include <exception>
#include <string>

namespace rlx_money {

/// @brief 金钱系统异常类
class MoneyException : public std::exception {
public:
    /// @brief 构造函数
    /// @param code 错误码
    /// @param message 错误消息
    explicit MoneyException(ErrorCode code, const std::string& message);

    /// @brief 获取错误消息
    /// @return 错误消息字符串
    [[nodiscard]] const char* what() const noexcept override;

    /// @brief 获取错误码
    /// @return 错误码
    [[nodiscard]] ErrorCode getErrorCode() const noexcept { return mCode; }

    /// @brief 获取详细错误信息
    /// @return 包含错误码和消息的详细信息
    [[nodiscard]] std::string getDetailedMessage() const;

private:
    ErrorCode           mCode;
    std::string         mMessage;
    mutable std::string mWhatMessage; // 缓存what()的返回值
};

/// @brief 数据库异常类
class DatabaseException : public MoneyException {
public:
    /// @brief 构造函数
    /// @param message 错误消息
    explicit DatabaseException(const std::string& message)
    : MoneyException(ErrorCode::DATABASE_ERROR, "数据库错误: " + message) {}
};

/// @brief 配置异常类
class ConfigException : public MoneyException {
public:
    /// @brief 构造函数
    /// @param message 错误消息
    explicit ConfigException(const std::string& message)
    : MoneyException(ErrorCode::CONFIG_ERROR, "配置错误: " + message) {}
};

/// @brief 权限异常类
class PermissionException : public MoneyException {
public:
    /// @brief 构造函数
    /// @param message 错误消息
    explicit PermissionException(const std::string& message)
    : MoneyException(ErrorCode::PERMISSION_DENIED, "权限错误: " + message) {}
};

/// @brief 参数异常类
class InvalidArgumentException : public MoneyException {
public:
    /// @brief 构造函数
    /// @param message 错误消息
    explicit InvalidArgumentException(const std::string& message)
    : MoneyException(ErrorCode::INVALID_AMOUNT, "参数错误: " + message) {}
};

} // namespace rlx_money