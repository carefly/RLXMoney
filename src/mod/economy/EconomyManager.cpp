#include "mod/economy/EconomyManager.h"
#include "mod/config/MoneyConfig.h"
#include "mod/database/DatabaseManager.h"
#include "mod/exceptions/MoneyException.h"
#include <RLXMoney/types/Types.h>
#include <SQLiteCpp/Statement.h>
#include <chrono>
#include <cmath>
#include <limits>
#include <random>


namespace rlx_money {

EconomyManager::EconomyManager()
: mPlayerDAO(DatabaseManager::getInstance()),
  mTransactionDAO(DatabaseManager::getInstance()),
  mInitialized(false) {
    // 构造函数中初始化依赖的单例，确保依赖关系正确
    try {
        // 确保依赖的单例已初始化
        (void)MoneyConfig::get(); // 确保配置已初始化
        DatabaseManager::getInstance();
    } catch (const std::exception&) {
        // 依赖初始化失败，抛出异常
        throw;
    }
}

EconomyManager& EconomyManager::getInstance() {
    static EconomyManager instance;
    return instance;
}

bool EconomyManager::initialize() {
    // 单线程模式，无需加锁

    // 如果已初始化，直接返回成功
    if (mInitialized) {
        return true;
    }

    try {
        // 读取配置并确保数据库已初始化
        const auto& config = MoneyConfig::get();

        auto& dbManager = DatabaseManager::getInstance();
        if (!dbManager.isInitialized()) {
            if (!dbManager.initialize(config.database.path)) {
                return false;
            }
        }

        // 同步配置文件中的币种到数据库
        if (!syncCurrenciesFromConfig()) {
            return false;
        }

        // 标记初始化完成
        mInitialized = true;
        return true;

    } catch (const std::exception&) {
        return false;
    }
}

std::optional<int> EconomyManager::getBalance(const std::string& xuid, const std::string& currencyId) const {
    // 单线程模式，无需加锁

    try {
        if (!isValidCurrency(currencyId)) {
            throw InvalidArgumentException("无效的币种ID: " + currencyId);
        }

        return mPlayerDAO.getBalance(xuid, currencyId);

    } catch (const std::exception& e) {
        throw DatabaseException("获取玩家余额失败: " + std::string(e.what()));
    }
}

std::vector<PlayerBalance> EconomyManager::getAllBalances(const std::string& xuid) const {
    try {
        return mPlayerDAO.getAllBalances(xuid);
    } catch (const std::exception& e) {
        throw DatabaseException("获取玩家所有余额失败: " + std::string(e.what()));
    }
}

bool EconomyManager::setBalance(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 单线程模式，无需加锁

    try {
        // 检查玩家是否存在
        if (!mPlayerDAO.playerExists(xuid)) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "玩家不存在");
        }

        // 获取币种配置，检查最大余额限制
        const auto& config     = MoneyConfig::get();
        auto  currencyIt = config.currencies.find(currencyId);
        if (currencyIt == config.currencies.end()) {
            throw InvalidArgumentException("币种配置不存在: " + currencyId);
        }

        if (amount > currencyIt->second.maxBalance) {
            throw InvalidArgumentException("金额超过最大余额限制");
        }

        // 使用事务确保余额更新和交易记录创建的原子性
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db; // 避免未使用参数警告
                // 更新余额
                if (!mPlayerDAO.updateBalance(xuid, currencyId, amount)) {
                    return false;
                }

                // 创建交易记录
                return createTransactionRecord(xuid, currencyId, amount, amount, TransactionType::SET, description);

            } catch (const std::exception&) {
                // 事务会自动回滚
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("设置玩家余额失败: " + std::string(e.what()));
    }
}

bool EconomyManager::addMoney(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 单线程模式，无需加锁

    try {
        // 获取币种配置
        const auto& config     = MoneyConfig::get();
        auto  currencyIt = config.currencies.find(currencyId);
        if (currencyIt == config.currencies.end()) {
            throw InvalidArgumentException("币种配置不存在: " + currencyId);
        }

        // 检查玩家是否存在
        if (!mPlayerDAO.playerExists(xuid)) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "玩家不存在，请先初始化玩家");
        }

        // 获取当前余额
        auto currentBalance = mPlayerDAO.getBalance(xuid, currencyId);
        int  oldBalance     = currentBalance.has_value() ? currentBalance.value() : 0;

        // 初始化余额（如果不存在）- 这种情况发生在玩家已存在但某个币种余额未初始化（例如添加了新币种）
        if (!currentBalance.has_value()) {
            oldBalance = currencyIt->second.initialBalance;
            mPlayerDAO.initializeBalance(xuid, currencyId, oldBalance);
        }

        int newBalance = oldBalance + amount;

        // 检查最大余额限制
        if (newBalance > currencyIt->second.maxBalance) {
            throw InvalidArgumentException("金额超过最大余额限制");
        }

        // 使用事务确保余额更新和交易记录创建的原子性
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db; // 避免未使用参数警告

                // 更新余额
                if (!mPlayerDAO.updateBalance(xuid, currencyId, newBalance)) {
                    return false;
                }

                // 创建交易记录
                return createTransactionRecord(xuid, currencyId, amount, newBalance, TransactionType::ADD, description);

            } catch (const std::exception&) {
                // 事务会自动回滚
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("增加玩家金钱失败: " + std::string(e.what()));
    }
}

bool EconomyManager::reduceMoney(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 单线程模式，无需加锁

    try {
        // 获取当前余额
        auto currentBalance = mPlayerDAO.getBalance(xuid, currencyId);
        if (!currentBalance.has_value()) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "玩家不存在或余额未初始化");
        }

        int oldBalance = currentBalance.value();

        // 检查余额是否充足
        if (oldBalance < amount) {
            throw MoneyException(ErrorCode::INSUFFICIENT_BALANCE, "余额不足");
        }

        int newBalance = oldBalance - amount;

        // 使用事务确保余额更新和交易记录创建的原子性
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db; // 避免未使用参数警告

                // 更新余额
                if (!mPlayerDAO.updateBalance(xuid, currencyId, newBalance)) {
                    return false;
                }

                // 创建交易记录
                return createTransactionRecord(
                    xuid,
                    currencyId,
                    -amount,
                    newBalance,
                    TransactionType::REDUCE,
                    description
                );

            } catch (const std::exception&) {
                // 事务会自动回滚
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("扣除玩家金钱失败: " + std::string(e.what()));
    }
}

bool EconomyManager::transferMoney(
    const std::string& fromXuid,
    const std::string& toXuid,
    const std::string& currencyId,
    int                amount,
    const std::string& description
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的转账金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 检查不能转账给自己
    if (fromXuid == toXuid) {
        throw InvalidArgumentException("不能转账给自己");
    }

    // 单线程模式，无需加锁

    try {
        // 获取币种配置
        const auto& config     = MoneyConfig::get();
        auto  currencyIt = config.currencies.find(currencyId);
        if (currencyIt == config.currencies.end()) {
            throw InvalidArgumentException("币种配置不存在: " + currencyId);
        }
        const auto& currency = currencyIt->second;

        // 检查转账是否允许
        if (!currency.allowPlayerTransfer) {
            throw MoneyException(ErrorCode::TRANSFER_DISABLED, "该币种不允许玩家转账");
        }

        if (amount < currency.minTransferAmount) {
            throw InvalidArgumentException("转账金额小于最小限制");
        }

        // 获取转出玩家余额
        auto fromBalance = mPlayerDAO.getBalance(fromXuid, currencyId);
        if (!fromBalance.has_value()) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "转出玩家不存在或余额未初始化");
        }

        // 获取转入玩家信息
        auto toPlayer = mPlayerDAO.getPlayerByXuid(toXuid);
        if (!toPlayer) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "转入玩家不存在");
        }

        // 获取转入玩家余额（如果不存在则初始化为初始余额）
        // 注意：这种情况发生在玩家已存在但某个币种余额未初始化（例如添加了新币种）
        auto toBalance    = mPlayerDAO.getBalance(toXuid, currencyId);
        int  toOldBalance = toBalance.has_value() ? toBalance.value() : 0;
        if (!toBalance.has_value()) {
            mPlayerDAO.initializeBalance(toXuid, currencyId, currency.initialBalance);
            toOldBalance = currency.initialBalance;
        }

        // 检查余额是否充足
        if (fromBalance.value() < amount) {
            throw MoneyException(ErrorCode::INSUFFICIENT_BALANCE, "余额不足");
        }

        // 计算手续费
        int fee = currency.transferFee;
        if (currency.feePercentage > 0.0) {
            double feeAmount  = static_cast<double>(amount) * currency.feePercentage / 100.0;
            fee              += static_cast<int>(std::round(feeAmount));
        }

        // 检查整数溢出风险
        if (amount > 0 && fee > 0 && amount > std::numeric_limits<int>::max() - fee) {
            throw InvalidArgumentException("转账金额和手续费过大，超出系统处理范围");
        }

        int totalAmount = amount + fee;

        // 检查余额是否充足（考虑手续费）
        if (fromBalance.value() < totalAmount) {
            throw MoneyException(ErrorCode::INSUFFICIENT_BALANCE, "余额不足（含手续费）");
        }

        // 检查减法溢出风险
        if (totalAmount > fromBalance.value()) {
            throw MoneyException(ErrorCode::INSUFFICIENT_BALANCE, "余额不足（含手续费）");
        }

        int fromNewBalance = fromBalance.value() - totalAmount;

        // 检查加法溢出风险
        if (amount > 0 && toOldBalance > std::numeric_limits<int>::max() - amount) {
            throw InvalidArgumentException("转入金额过大，超出系统处理范围");
        }

        int toNewBalance = toOldBalance + amount;

        // 检查最大余额限制
        if (toNewBalance > currency.maxBalance) {
            throw InvalidArgumentException("转入金额超过最大余额限制");
        }

        // 使用事务执行转账
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database&) -> bool {
            try {
                // 生成 transfer_id
                auto genId = []() {
                    static const char*                 hex = "0123456789abcdef";
                    std::string                        s(24, '0');
                    std::random_device                 rd;
                    std::mt19937                       rng(rd());
                    std::uniform_int_distribution<int> dist(0, 15);
                    for (char& c : s) c = hex[dist(rng)];
                    return s;
                };
                std::string transferId = genId();

                // 更新转出玩家余额
                if (!mPlayerDAO.updateBalance(fromXuid, currencyId, fromNewBalance)) {
                    return false;
                }

                // 更新转入玩家余额
                if (!mPlayerDAO.updateBalance(toXuid, currencyId, toNewBalance)) {
                    return false;
                }

                // 创建转出交易记录
                createTransactionRecord(
                    fromXuid,
                    currencyId,
                    -totalAmount,
                    fromNewBalance,
                    TransactionType::TRANSFER,
                    description,
                    toXuid,
                    transferId
                );

                // 创建转入交易记录
                createTransactionRecord(
                    toXuid,
                    currencyId,
                    amount,
                    toNewBalance,
                    TransactionType::TRANSFER,
                    description,
                    fromXuid,
                    transferId
                );

                return true;

            } catch (const std::exception&) {
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("转账失败: " + std::string(e.what()));
    }
}

bool EconomyManager::initializeNewPlayer(const std::string& xuid, const std::string& username) {
    // 单线程模式，无需加锁

    try {
        // 步骤1：玩家存在性检查（必须在事务外执行）
        if (mPlayerDAO.playerExists(xuid)) {
            throw MoneyException(ErrorCode::PLAYER_ALREADY_EXISTS, "玩家已存在");
        }

        // 步骤2：整个初始化过程在事务中执行
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db;                                    // 避免未使用参数警告
                int64_t currentTime = getCurrentTimestamp(); // 时间戳保持 int64_t

                // 2.1 创建玩家基础记录（players表）
                PlayerData playerData(xuid, username, currentTime);
                playerData.createdAt = currentTime;
                playerData.updatedAt = currentTime;

                if (!mPlayerDAO.createPlayer(playerData)) {
                    return false;
                }

                // 2.2 为所有启用的币种初始化余额和交易记录
                const auto& config = MoneyConfig::get();
                for (const auto& [currencyId, currency] : config.currencies) {
                    if (currency.enabled) {
                        // 直接在事务中执行余额初始化（player_balances表）
                        SQLite::Statement balanceStmt(
                            db,
                            "INSERT INTO player_balances (xuid, currency_id, balance, updated_at) VALUES (?, ?, ?, ?)"
                        );
                        balanceStmt.bind(1, xuid);
                        balanceStmt.bind(2, currencyId);
                        balanceStmt.bind(3, currency.initialBalance);
                        balanceStmt.bind(4, currentTime);
                        balanceStmt.exec();

                        // 创建交易记录（transactions表）
                        createTransactionRecord(
                            xuid,
                            currencyId,
                            currency.initialBalance,
                            currency.initialBalance,
                            TransactionType::INITIAL,
                            "新玩家初始金额"
                        );
                    }
                }

                return true;

            } catch (const std::exception&) {
                // 任何步骤失败都会自动回滚，确保不会有部分创建的玩家
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("初始化新玩家失败: " + std::string(e.what()));
    }
}

bool EconomyManager::playerExists(const std::string& xuid) const { return mPlayerDAO.playerExists(xuid); }

std::vector<TopBalanceEntry> EconomyManager::getTopBalanceList(const std::string& currencyId, int limit) const {
    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }
    return mPlayerDAO.getTopBalanceList(currencyId, limit);
}

std::vector<TransactionRecord>
EconomyManager::getPlayerTransactions(const std::string& xuid, const std::string& currencyId, int page, int pageSize)
    const {
    return mTransactionDAO.getPlayerTransactions(xuid, currencyId, page, pageSize);
}

int EconomyManager::getPlayerTransactionCount(const std::string& xuid) const {
    return mTransactionDAO.getPlayerTransactionCount(xuid);
}

bool EconomyManager::isValidAmount(int amount) const { return amount >= 0; }

bool EconomyManager::hasSufficientBalance(const std::string& xuid, const std::string& currencyId, int amount) const {
    auto balance = getBalance(xuid, currencyId);
    return balance.has_value() && balance.value() >= amount;
}

int EconomyManager::getTotalWealth(const std::string& currencyId) const {
    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }
    return mPlayerDAO.getTotalWealth(currencyId);
}

int EconomyManager::getPlayerCount() const { return mPlayerDAO.getPlayerCount(); }

bool EconomyManager::createTransactionRecord(
    const std::string&                xuid,
    const std::string&                currencyId,
    int                               amount,
    int                               balance,
    TransactionType                   type,
    const std::string&                description,
    const std::optional<std::string>& relatedXuid,
    const std::optional<std::string>& transferId
) {
    try {
        // 如果没有提供描述，根据交易类型自动生成
        std::string finalDescription = description;
        if (finalDescription.empty()) {
            // 获取关联玩家名称（如果有的话）
            std::string relatedPlayerName;
            if (relatedXuid.has_value()) {
                auto relatedPlayer = mPlayerDAO.getPlayerByXuid(relatedXuid.value());
                if (relatedPlayer) {
                    relatedPlayerName = relatedPlayer->username;
                }
            }

            auto amountAbs = std::abs(amount);

            MoneyFlow flow = MoneyFlow::NEUTRAL;
            switch (type) {
            case TransactionType::SET:
            case TransactionType::INITIAL:
                flow = MoneyFlow::NEUTRAL;
                break;
            case TransactionType::ADD:
                flow = (amount >= 0) ? MoneyFlow::CREDIT : MoneyFlow::DEBIT;
                break;
            case TransactionType::REDUCE:
                flow = MoneyFlow::DEBIT;
                break;
            case TransactionType::TRANSFER:
                flow = (amount >= 0) ? MoneyFlow::CREDIT : MoneyFlow::DEBIT;
                break;
            default:
                flow = MoneyFlow::NEUTRAL;
                break;
            }

            finalDescription = describe(type, amountAbs, flow, relatedPlayerName);
        }

        TransactionRecord record(
            0,
            xuid,
            currencyId,
            amount,
            balance,
            type,
            finalDescription,
            getCurrentTimestamp(),
            relatedXuid,
            transferId
        );
        return mTransactionDAO.createTransaction(record);

    } catch (const std::exception& e) {
        throw DatabaseException("创建交易记录失败: " + std::string(e.what()));
    }
}

bool EconomyManager::setBalance(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    OperatorType       operatorType,
    const std::string& operatorName
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 单线程模式，无需加锁

    try {
        // 检查玩家是否存在
        if (!mPlayerDAO.playerExists(xuid)) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "玩家不存在");
        }

        // 获取币种配置，检查最大余额限制
        const auto& config     = MoneyConfig::get();
        auto  currencyIt = config.currencies.find(currencyId);
        if (currencyIt == config.currencies.end()) {
            throw InvalidArgumentException("币种配置不存在: " + currencyId);
        }

        if (amount > currencyIt->second.maxBalance) {
            throw InvalidArgumentException("金额超过最大余额限制");
        }

        // 生成带操作者信息的描述
        std::string description = describe(
            TransactionType::SET,
            static_cast<uint64_t>(amount),
            MoneyFlow::NEUTRAL,
            operatorType,
            operatorName
        );

        // 使用事务确保余额更新和交易记录创建的原子性
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db; // 避免未使用参数警告

                // 更新余额
                if (!mPlayerDAO.updateBalance(xuid, currencyId, amount)) {
                    return false;
                }

                // 创建交易记录
                return createTransactionRecord(xuid, currencyId, amount, amount, TransactionType::SET, description);

            } catch (const std::exception&) {
                // 事务会自动回滚
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("设置玩家余额失败: " + std::string(e.what()));
    }
}

bool EconomyManager::addMoney(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    OperatorType       operatorType,
    const std::string& operatorName
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 单线程模式，无需加锁

    try {
        // 获取币种配置
        const auto& config     = MoneyConfig::get();
        auto  currencyIt = config.currencies.find(currencyId);
        if (currencyIt == config.currencies.end()) {
            throw InvalidArgumentException("币种配置不存在: " + currencyId);
        }

        // 检查玩家是否存在
        if (!mPlayerDAO.playerExists(xuid)) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "玩家不存在，请先初始化玩家");
        }

        // 获取当前余额
        auto currentBalance = mPlayerDAO.getBalance(xuid, currencyId);
        int  oldBalance     = currentBalance.has_value() ? currentBalance.value() : 0;

        // 初始化余额（如果不存在）- 这种情况发生在玩家已存在但某个币种余额未初始化（例如添加了新币种）
        if (!currentBalance.has_value()) {
            oldBalance = currencyIt->second.initialBalance;
            mPlayerDAO.initializeBalance(xuid, currencyId, oldBalance);
        }

        int newBalance = oldBalance + amount;

        // 检查最大余额限制
        if (newBalance > currencyIt->second.maxBalance) {
            throw InvalidArgumentException("金额超过最大余额限制");
        }

        // 生成带操作者信息的描述
        std::string description = describe(
            TransactionType::ADD,
            static_cast<uint64_t>(amount),
            MoneyFlow::CREDIT,
            operatorType,
            operatorName
        );

        // 使用事务确保余额更新和交易记录创建的原子性
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db; // 避免未使用参数警告

                // 更新余额
                if (!mPlayerDAO.updateBalance(xuid, currencyId, newBalance)) {
                    return false;
                }

                // 创建交易记录
                return createTransactionRecord(xuid, currencyId, amount, newBalance, TransactionType::ADD, description);

            } catch (const std::exception&) {
                // 事务会自动回滚
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("增加玩家金钱失败: " + std::string(e.what()));
    }
}

bool EconomyManager::reduceMoney(
    const std::string& xuid,
    const std::string& currencyId,
    int                amount,
    OperatorType       operatorType,
    const std::string& operatorName
) {
    if (!isValidAmount(amount)) {
        throw InvalidArgumentException("无效的金额");
    }

    if (!isValidCurrency(currencyId)) {
        throw InvalidArgumentException("无效的币种ID: " + currencyId);
    }

    // 单线程模式，无需加锁

    try {
        // 获取当前余额
        auto currentBalance = mPlayerDAO.getBalance(xuid, currencyId);
        if (!currentBalance.has_value()) {
            throw MoneyException(ErrorCode::PLAYER_NOT_FOUND, "玩家不存在或余额未初始化");
        }

        int oldBalance = currentBalance.value();

        // 检查余额是否充足
        if (oldBalance < amount) {
            throw MoneyException(ErrorCode::INSUFFICIENT_BALANCE, "余额不足");
        }

        int newBalance = oldBalance - amount;

        // 生成带操作者信息的描述
        std::string description = describe(
            TransactionType::REDUCE,
            static_cast<uint64_t>(amount),
            MoneyFlow::DEBIT,
            operatorType,
            operatorName
        );

        // 使用事务确保余额更新和交易记录创建的原子性
        return DatabaseManager::getInstance().executeTransaction([&](SQLite::Database& db) -> bool {
            try {
                (void)db; // 避免未使用参数警告

                // 更新余额
                if (!mPlayerDAO.updateBalance(xuid, currencyId, newBalance)) {
                    return false;
                }

                // 创建交易记录
                return createTransactionRecord(
                    xuid,
                    currencyId,
                    -amount,
                    newBalance,
                    TransactionType::REDUCE,
                    description
                );

            } catch (const std::exception&) {
                // 事务会自动回滚
                return false;
            }
        });

    } catch (const std::exception& e) {
        throw DatabaseException("扣除玩家金钱失败: " + std::string(e.what()));
    }
}

int64_t EconomyManager::getCurrentTimestamp() const {
    return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

std::string EconomyManager::getDefaultCurrencyId() const {
    return MoneyConfig::get().defaultCurrency;
}

bool EconomyManager::syncCurrenciesFromConfig() {
    // Currency 现在只存储在配置文件中，不需要同步到数据库
    // 这个方法保留用于 reload 命令的调用，但不需要执行任何操作
    return true;
}

bool EconomyManager::isValidCurrency(const std::string& currencyId) const {
    if (currencyId.empty()) {
        return false;
    }
    // 从配置文件检查币种是否存在且启用
    const auto& config     = MoneyConfig::get();
    auto        currencyIt = config.currencies.find(currencyId);
    return currencyIt != config.currencies.end() && currencyIt->second.enabled;
}

void EconomyManager::resetForTesting() {
    // 重置初始化状态，允许重新初始化
    mInitialized = false;
}

} // namespace rlx_money