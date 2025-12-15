#pragma once

#include <string>

namespace rlx_money {

/// @brief 交易类型枚举
enum class TransactionType {
    SET,      // 设置余额
    ADD,      // 增加金钱
    REDUCE,   // 减少金钱
    TRANSFER, // 转账
    INITIAL   // 初始金额
};

/// @brief 操作者类型枚举
enum class OperatorType {
    ADMIN,       // 管理员
    SHOP,        // 商店
    REAL_ESTATE, // 地产商
    SYSTEM,      // 系统
    PLAYER,      // 玩家
    OTHER        // 其他
};

/// @brief 资金流向（会计极性）
enum class MoneyFlow {
    CREDIT, // 进账
    DEBIT,  // 出账
    NEUTRAL // 中性（不涉及进/出）
};

/// @brief 错误码枚举
enum class ErrorCode {
    SUCCESS = 0,
    PLAYER_NOT_FOUND,
    INSUFFICIENT_BALANCE,
    INVALID_AMOUNT,
    DATABASE_ERROR,
    PERMISSION_DENIED,
    TRANSFER_DISABLED,
    CONFIG_ERROR,
    PLAYER_ALREADY_EXISTS
};

/// @brief 交易类型转换为字符串
/// @param type 交易类型
/// @return 字符串表示
[[nodiscard]] std::string transactionTypeToString(TransactionType type);

/// @brief 字符串转换为交易类型
/// @param typeStr 字符串
/// @return 交易类型
[[nodiscard]] TransactionType stringToTransactionType(const std::string& typeStr);

/// @brief 错误码转换为字符串
/// @param code 错误码
/// @return 字符串表示
[[nodiscard]] std::string errorCodeToString(ErrorCode code);

/// @brief 操作者类型转换为字符串
/// @param type 操作者类型
/// @return 字符串表示
[[nodiscard]] std::string operatorTypeToString(OperatorType type);

/// @brief 生成默认的交易描述（非兼容重构版）
/// @param type 交易类型
/// @param amountAbs 金额（非负，最小单位）
/// @param flow 资金流向（CREDIT/DEBIT/NEUTRAL）
/// @param relatedPlayerName 关联玩家名称（可选）
/// @return 默认描述
[[nodiscard]] std::string describe(TransactionType type, uint64_t amountAbs, MoneyFlow flow, const std::string& relatedPlayerName = "");

/// @brief 生成带操作者信息的交易描述（非兼容重构版）
/// @param type 交易类型
/// @param amountAbs 金额（非负，最小单位）
/// @param flow 资金流向（CREDIT/DEBIT/NEUTRAL）
/// @param operatorType 操作者类型
/// @param operatorName 操作者名称
/// @param relatedPlayerName 关联玩家名称（可选）
/// @return 带操作者信息的描述
[[nodiscard]] std::string describe(
    TransactionType type,
    uint64_t amountAbs,
    MoneyFlow flow,
    OperatorType operatorType,
    const std::string& operatorName,
    const std::string& relatedPlayerName = ""
);

} // namespace rlx_money


