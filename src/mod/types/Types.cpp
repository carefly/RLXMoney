#include <RLXMoney/types/Types.h>
#include <stdexcept>

namespace rlx_money {

std::string transactionTypeToString(TransactionType type) {
    switch (type) {
    case TransactionType::SET:
        return "set";
    case TransactionType::ADD:
        return "add";
    case TransactionType::REDUCE:
        return "reduce";
    case TransactionType::TRANSFER:
        return "transfer";
    case TransactionType::INITIAL:
        return "initial";
    default:
        return "unknown";
    }
}

TransactionType stringToTransactionType(const std::string& typeStr) {
    if (typeStr == "set") {
        return TransactionType::SET;
    } else if (typeStr == "add") {
        return TransactionType::ADD;
    } else if (typeStr == "reduce") {
        return TransactionType::REDUCE;
    } else if (typeStr == "transfer") {
        return TransactionType::TRANSFER;
    } else if (typeStr == "initial") {
        return TransactionType::INITIAL;
    } else {
        throw std::invalid_argument("无效的交易类型: " + typeStr);
    }
}

std::string errorCodeToString(ErrorCode code) {
    switch (code) {
    case ErrorCode::SUCCESS:
        return "成功";
    case ErrorCode::PLAYER_NOT_FOUND:
        return "玩家不存在";
    case ErrorCode::INSUFFICIENT_BALANCE:
        return "余额不足";
    case ErrorCode::INVALID_AMOUNT:
        return "无效金额";
    case ErrorCode::DATABASE_ERROR:
        return "数据库错误";
    case ErrorCode::PERMISSION_DENIED:
        return "权限不足";
    case ErrorCode::TRANSFER_DISABLED:
        return "转账功能已禁用";
    case ErrorCode::CONFIG_ERROR:
        return "配置错误";
    case ErrorCode::PLAYER_ALREADY_EXISTS:
        return "玩家已存在";
    default:
        return "未知错误";
    }
}

std::string operatorTypeToString(OperatorType type) {
    switch (type) {
    case OperatorType::ADMIN:
        return "管理员";
    case OperatorType::SHOP:
        return "商店";
    case OperatorType::REAL_ESTATE:
        return "地产商";
    case OperatorType::SYSTEM:
        return "系统";
    case OperatorType::PLAYER:
        return "玩家";
    case OperatorType::OTHER:
        return "其他";
    default:
        return "未知";
    }
}

// 新的描述生成：非负金额 + 流向
static std::string describeCore(
    TransactionType type,
    uint64_t amountAbs,
    MoneyFlow flow,
    const std::string& relatedPlayerName
) {
    switch (type) {
    case TransactionType::SET:
        return "管理员设置余额为 " + std::to_string(amountAbs);
    case TransactionType::ADD:
        return (flow == MoneyFlow::CREDIT)
            ? ("获得 " + std::to_string(amountAbs) + " 金币")
            : ("扣除 " + std::to_string(amountAbs) + " 金币");
    case TransactionType::REDUCE:
        return "消费 " + std::to_string(amountAbs) + " 金币";
    case TransactionType::TRANSFER:
        if (!relatedPlayerName.empty()) {
            return (flow == MoneyFlow::CREDIT)
                ? ("从 " + relatedPlayerName + " 收到转账 " + std::to_string(amountAbs) + " 金币")
                : ("向 " + relatedPlayerName + " 转账 " + std::to_string(amountAbs) + " 金币");
        }
        return "转账 " + std::to_string(amountAbs) + " 金币";
    case TransactionType::INITIAL:
        return "新玩家初始金额 " + std::to_string(amountAbs);
    default:
        return "交易 " + std::to_string(amountAbs) + " 金币";
    }
}

std::string describe(TransactionType type, uint64_t amountAbs, MoneyFlow flow, const std::string& relatedPlayerName) {
    return describeCore(type, amountAbs, flow, relatedPlayerName);
}

std::string describe(
    TransactionType type,
    uint64_t amountAbs,
    MoneyFlow flow,
    OperatorType operatorType,
    const std::string& operatorName,
    const std::string& relatedPlayerName
) {
    auto withOp = [&](const std::string& prefix, const std::string& body) -> std::string {
        if (operatorName.empty()) {
            return prefix + operatorTypeToString(operatorType) + body;
        }
        return prefix + operatorTypeToString(operatorType) + "[" + operatorName + "]" + body;
    };

    switch (type) {
    case TransactionType::SET:
        return withOp("", "设置余额为 " + std::to_string(amountAbs));
    case TransactionType::ADD:
        if (flow == MoneyFlow::CREDIT) {
            return withOp("从", "获得 " + std::to_string(amountAbs) + " 金币");
        }
        return withOp("被", "扣除 " + std::to_string(amountAbs) + " 金币");
    case TransactionType::REDUCE:
        return withOp("向", "消费 " + std::to_string(amountAbs) + " 金币");
    case TransactionType::TRANSFER:
        if (!relatedPlayerName.empty()) {
            return (flow == MoneyFlow::CREDIT)
                ? ("从 " + relatedPlayerName + " 收到转账 " + std::to_string(amountAbs) + " 金币")
                : ("向 " + relatedPlayerName + " 转账 " + std::to_string(amountAbs) + " 金币");
        }
        return "转账 " + std::to_string(amountAbs) + " 金币";
    case TransactionType::INITIAL:
        return "新玩家初始金额 " + std::to_string(amountAbs);
    default:
        return withOp("", "交易 " + std::to_string(amountAbs) + " 金币");
    }
}

} // namespace rlx_money