#pragma once

#include <RLXMoney/types/Types.h>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace rlx_money {

/// @brief 玩家数据结构
struct PlayerData {
    std::string xuid;          // 玩家唯一标识
    std::string username;      // 玩家用户名
    int64_t     firstJoinTime; // 首次加入时间
    int64_t     createdAt;     // 记录创建时间
    int64_t     updatedAt;     // 记录更新时间

    /// @brief 构造函数
    PlayerData() : firstJoinTime(0), createdAt(0), updatedAt(0) {}

    /// @brief 构造函数
    /// @param x 玩家XUID
    /// @param name 玩家用户名
    /// @param joinTime 首次加入时间
    PlayerData(std::string x, std::string name, int64_t joinTime = 0)
    : xuid(std::move(x)),
      username(std::move(name)),
      firstJoinTime(joinTime),
      createdAt(joinTime),
      updatedAt(joinTime) {}
};

/// @brief 玩家币种余额结构
struct PlayerBalance {
    std::string xuid;       // 玩家XUID
    std::string currencyId; // 币种ID
    int         balance;    // 余额
    int64_t     updatedAt;  // 更新时间

    /// @brief 构造函数
    PlayerBalance() : balance(0), updatedAt(0) {}

    /// @brief 构造函数
    /// @param x 玩家XUID
    /// @param cid 币种ID
    /// @param bal 余额
    PlayerBalance(std::string x, std::string cid, int bal = 0)
    : xuid(std::move(x)),
      currencyId(std::move(cid)),
      balance(bal),
      updatedAt(0) {}
};

/// @brief 交易记录结构
struct TransactionRecord {
    int64_t                    id;          // 记录ID
    std::string                xuid;        // 玩家XUID
    std::string                currencyId;  // 币种ID
    int                        amount;      // 交易金额
    int                        balance;     // 交易后余额
    TransactionType            type;        // 交易类型
    std::string                description; // 交易描述
    int64_t                    timestamp;   // 交易时间戳
    std::optional<std::string> relatedXuid; // 关联玩家XUID（转账时使用）
    std::optional<std::string> transferId;  // 转账ID（出入两条记录共享）

    /// @brief 构造函数
    TransactionRecord() : id(0), amount(0), balance(0), type(TransactionType::SET), timestamp(0) {}

    /// @brief 构造函数
    /// @param recordId 记录ID
    /// @param x 玩家XUID
    /// @param cid 币种ID
    /// @param amt 交易金额
    /// @param bal 交易后余额
    /// @param t 交易类型
    /// @param desc 交易描述
    /// @param ts 时间戳
    /// @param relatedX 关联玩家XUID
    TransactionRecord(
        int64_t                           recordId,
        std::string                       x,
        std::string                       cid,
        int                               amt,
        int                               bal,
        TransactionType                   t,
        std::string                       desc     = "",
        int64_t                           ts       = 0,
        const std::optional<std::string>& relatedX = std::nullopt,
        const std::optional<std::string>& transfer = std::nullopt
    )
    : id(recordId),
      xuid(std::move(x)),
      currencyId(std::move(cid)),
      amount(amt),
      balance(bal),
      type(t),
      description(std::move(desc)),
      timestamp(ts),
      relatedXuid(relatedX),
      transferId(transfer) {}
};

/// @brief 财富排行榜条目
struct TopBalanceEntry {
    std::string username;   // 玩家用户名
    std::string xuid;       // 玩家XUID
    std::string currencyId; // 币种ID
    int         balance;    // 余额
    int         rank;       // 排名

    /// @brief 构造函数
    TopBalanceEntry() : balance(0), rank(0) {}

    /// @brief 构造函数
    /// @param name 玩家用户名
    /// @param x 玩家XUID
    /// @param cid 币种ID
    /// @param bal 余额
    /// @param r 排名
    TopBalanceEntry(std::string name, std::string x, std::string cid, int bal, int r)
    : username(std::move(name)),
      xuid(std::move(x)),
      currencyId(std::move(cid)),
      balance(bal),
      rank(r) {}
};

} // namespace rlx_money


