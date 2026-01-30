#include "Commands.h"
#include "mod/api/LeviLaminaAPI.h"
#include "mod/config/MoneyConfig.h"
#include "mod/economy/EconomyManager.h"
#include "mod/exceptions/MoneyException.h"


#include <cstdint>
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandHandle.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/service/Service.h>
#include <mc/server/ServerPlayer.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandRawText.h>
#include <mc/world/actor/Actor.h>
#include <mc/world/actor/player/Player.h>
#include <mc/world/level/Level.h>


namespace rlx_money {

void Commands::registerCommands() {
    using ll::command::CommandRegistrar;
    auto& commad = CommandRegistrar::getInstance(false).getOrCreateCommand("money", "金钱");
    commad.overload<BasicCommand>()
        .required("Operation")
        .optional("Currency")
        .execute([](CommandOrigin const& origin, CommandOutput& output, BasicCommand const& param, Command const&) {
            auto operation = param.Operation;
            auto actor     = origin.getEntity();

            if (actor == nullptr || !actor->isType(ActorType::Player)) {
                output.error("只有玩家可以操作金钱");
                return;
            }
            auto player = static_cast<Player*>(actor);
            auto xuid   = player->getXuid();

            // 获取币种ID（如果未指定则使用默认币种）
            std::string currencyId = param.Currency.mText.empty() ? EconomyManager::getInstance().getDefaultCurrencyId()
                                                                  : param.Currency.mText;

            if (CommandBasicOperation::query == operation) {
                // 如果未指定币种，显示所有币种余额
                if (param.Currency.mText.empty()) {
                    auto balances = EconomyManager::getInstance().getAllBalances(xuid);
                    if (balances.empty()) {
                        player->sendMessage("§c没有找到任何币种余额");
                        return;
                    }

                    const auto& config = MoneyConfig::get();
                    player->sendMessage("§a你的所有币种余额：");
                    for (const auto& balance : balances) {
                        auto currencyIt = config.currencies.find(balance.currencyId);
                        if (currencyIt != config.currencies.end()) {
                            player->sendMessage(
                                fmt::format("§7- §b{}§7: §6{}", currencyIt->second.name, balance.balance)
                            );
                        }
                    }
                } else {
                    auto amount = EconomyManager::getInstance().getBalance(xuid, currencyId);
                    if (!amount.has_value()) {
                        output.error("数据异常或币种不存在，请联系腐竹");
                        return;
                    }

                    const auto& config     = MoneyConfig::get();
                    auto        currencyIt = config.currencies.find(currencyId);
                    std::string currencyName =
                        currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                    player->sendMessage(fmt::format("§a查询成功，§b{}§a 余额为 §6{}", currencyName, amount.value()));
                }
            } else if (CommandBasicOperation::history == operation) {
                auto history = EconomyManager::getInstance().getPlayerTransactions(xuid, currencyId);
                if (history.empty()) {
                    player->sendMessage("§e暂时没有交易记录");
                    return;
                }

                const auto& config       = MoneyConfig::get();
                auto        currencyIt   = config.currencies.find(currencyId);
                std::string currencyName = currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                player->sendMessage(fmt::format("§b{} §a交易记录：", currencyName));
                for (const auto& record : history) {
                    player->sendMessage(fmt::format(
                        "§7- {}，金额为 §6{}§7，余额为 §6{}",
                        record.description,
                        record.amount,
                        record.balance
                    ));
                }
            }
        });
    commad.overload<PayCommand>()
        .required("Operation")
        .required("Target")
        .required("Amount")
        .optional("Currency")
        .execute([](CommandOrigin const& origin, CommandOutput& output, PayCommand const& param, Command const&) {
            auto operation = param.Operation;
            auto target    = param.Target.mText;
            auto amount    = param.Amount;

            auto actor = origin.getEntity();

            if (actor == nullptr || !actor->isType(ActorType::Player)) {
                output.error("只有玩家可以执行转账操作");
                return;
            }

            if (CommandPayOperation::pay != operation) {
                output.error("无效的转账操作");
                return;
            }

            if (amount <= 0) {
                output.error("转账金额必须大于0");
                return;
            }

            auto player   = static_cast<Player*>(actor);
            auto fromXuid = player->getXuid();

            // 获取币种ID（如果未指定则使用默认币种）
            std::string currencyId = param.Currency.mText.empty() ? EconomyManager::getInstance().getDefaultCurrencyId()
                                                                  : param.Currency.mText;

            // 获取目标玩家
            auto targetPlayerName = target;
            auto targetXuid       = LeviLaminaAPI::getXuidByPlayerName(targetPlayerName);

            // 执行转账
            try {
                const auto& config       = MoneyConfig::get();
                auto        currencyIt   = config.currencies.find(currencyId);
                std::string currencyName = currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;

                std::string description = fmt::format("向 {} 转账", targetPlayerName);
                bool        success =
                    EconomyManager::getInstance().transferMoney(fromXuid, targetXuid, currencyId, amount, description);

                if (success) {
                    // 计算手续费
                    int64_t fee = 0;
                    if (currencyIt != config.currencies.end()) {
                        const auto& currency = currencyIt->second;
                        fee                  = currency.transferFee;
                        if (currency.feePercentage > 0.0) {
                            double feeAmount  = static_cast<double>(amount) * currency.feePercentage / 100.0;
                            fee              += static_cast<int>(std::round(feeAmount));
                        }
                    }

                    // 显示转账成功消息（包含手续费）
                    if (fee > 0) {
                        player->sendMessage(fmt::format(
                            "§a成功向 §e{}§a 转账 §6{} §b{}§a（手续费：§c{} §b{}§a）",
                            targetPlayerName,
                            amount,
                            currencyName,
                            fee,
                            currencyName
                        ));
                    } else {
                        player->sendMessage(
                            fmt::format("§a成功向 §e{}§a 转账 §6{} §b{}", targetPlayerName, amount, currencyName)
                        );
                    }

                    auto targetPlayer = LeviLaminaAPI::getPlayerByXuid(targetXuid);
                    if (targetPlayer) {
                        targetPlayer->sendMessage(fmt::format(
                            "§a收到来自 §e{}§a 的转账 §6{} §b{}",
                            std::string(player->mName),
                            amount,
                            currencyName
                        ));
                    }
                } else {
                    // 转账失败但没有抛出异常（通常是数据库操作失败）
                    player->sendMessage(fmt::format("§c转账失败：操作未能完成，请稍后重试或联系管理员"));
                    output.error("转账操作失败");
                }
            } catch (const std::exception& e) {
                player->sendMessage(fmt::format("§c转账失败：{}", e.what()));
            }
        });

    auto& opCommand = CommandRegistrar::getInstance(false)
                          .getOrCreateCommand("moneyop", "金钱管理", CommandPermissionLevel::GameDirectors);
    // 管理员命令处理函数
    opCommand.overload<AdminCommand>()
        .required("Operation")
        .optional("Target")
        .optional("Amount")
        .optional("Currency")
        .execute([](CommandOrigin const& origin, CommandOutput& output, AdminCommand const& param, Command const&) {
            // 辅助函数：验证玩家权限
            auto validatePlayer = [&origin, &output]() -> Player* {
                auto actor = origin.getEntity();
                if (actor == nullptr || !actor->isType(ActorType::Player)) {
                    output.error("只有玩家可以执行管理员操作");
                    return nullptr;
                }

                auto player = static_cast<Player*>(actor);
                if (!player->isOperator()) {
                    output.error("你没有权限执行管理员操作");
                    return nullptr;
                }
                return player;
            };

            // 辅助函数：获取目标玩家 XUID
            auto getTargetXuid = [&output](const std::string& targetName) -> std::string {
                if (targetName.empty()) {
                    output.error("请指定目标玩家");
                    return "";
                }

                auto targetXuid = LeviLaminaAPI::getXuidByPlayerName(targetName);
                if (targetXuid.empty()) {
                    output.error(fmt::format("找不到玩家 {}", targetName));
                }
                return targetXuid;
            };

            // 获取币种ID（如果未指定则使用默认币种）
            std::string currencyId = param.Currency.mText.empty() ? EconomyManager::getInstance().getDefaultCurrencyId()
                                                                  : param.Currency.mText;

            // 辅助函数：处理需要目标玩家的操作
            auto handleTargetedOperation =
                [&getTargetXuid, &validatePlayer, &output, &param, &currencyId](auto operationFunc) -> bool {
                auto player = validatePlayer();
                if (!player) return false;

                auto targetXuid = getTargetXuid(param.Target.mText);
                if (targetXuid.empty()) return false;

                try {
                    operationFunc(player, targetXuid, param.Amount, param.Target.mText, currencyId);
                    return true;
                } catch (const std::exception& e) {
                    output.error(fmt::format("操作失败：{}", e.what()));
                    return false;
                }
            };

            auto operation = param.Operation;

            try {
                switch (operation) {
                case CommandAdminOperation::set:
                    if (!handleTargetedOperation([](Player*            player,
                                                    const std::string& targetXuid,
                                                    int                amount,
                                                    const std::string& targetName,
                                                    const std::string& currencyId) {
                            EconomyManager::getInstance().setBalance(
                                targetXuid,
                                currencyId,
                                amount,
                                OperatorType::ADMIN,
                                std::string(player->mName)
                            );

                            const auto& config     = MoneyConfig::get();
                            auto        currencyIt = config.currencies.find(currencyId);
                            std::string currencyName =
                                currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                            player->sendMessage(fmt::format(
                                "§a成功将 §e{}§a 的 §b{}§a 余额设置为 §6{}",
                                targetName,
                                currencyName,
                                amount
                            ));
                        })) {
                        auto player = validatePlayer();
                        if (player) player->sendMessage("§c设置余额操作失败，请检查命令参数");
                    }
                    break;

                case CommandAdminOperation::give:
                    if (!handleTargetedOperation([](Player*            player,
                                                    const std::string& targetXuid,
                                                    int                amount,
                                                    const std::string& targetName,
                                                    const std::string& currencyId) {
                            EconomyManager::getInstance().addMoney(
                                targetXuid,
                                currencyId,
                                amount,
                                OperatorType::ADMIN,
                                std::string(player->mName)
                            );

                            const auto& config     = MoneyConfig::get();
                            auto        currencyIt = config.currencies.find(currencyId);
                            std::string currencyName =
                                currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                            player->sendMessage(
                                fmt::format("§a成功给予 §e{}§a §6{} §b{}", targetName, amount, currencyName)
                            );
                        })) {
                        auto player = validatePlayer();
                        if (player) player->sendMessage("§c给予金币操作失败，请检查命令参数");
                    }
                    break;

                case CommandAdminOperation::take:
                    if (!handleTargetedOperation([](Player*            player,
                                                    const std::string& targetXuid,
                                                    int                amount,
                                                    const std::string& targetName,
                                                    const std::string& currencyId) {
                            EconomyManager::getInstance().reduceMoney(
                                targetXuid,
                                currencyId,
                                amount,
                                OperatorType::ADMIN,
                                std::string(player->mName)
                            );

                            const auto& config     = MoneyConfig::get();
                            auto        currencyIt = config.currencies.find(currencyId);
                            std::string currencyName =
                                currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                            player->sendMessage(
                                fmt::format("§a成功从 §e{}§a 扣除 §6{} §b{}", targetName, amount, currencyName)
                            );
                        })) {
                        auto player = validatePlayer();
                        if (player) player->sendMessage("§c扣除金币操作失败，请检查命令参数");
                    }
                    break;

                case CommandAdminOperation::check:
                    if (!handleTargetedOperation([](Player*            player,
                                                    const std::string& targetXuid,
                                                    int /*amount*/,
                                                    const std::string& targetName,
                                                    const std::string& currencyId) {
                            auto balance = EconomyManager::getInstance().getBalance(targetXuid, currencyId);
                            if (balance.has_value()) {
                                const auto& config     = MoneyConfig::get();
                                auto        currencyIt = config.currencies.find(currencyId);
                                std::string currencyName =
                                    currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                                player->sendMessage(fmt::format(
                                    "§b{}§a 的 §b{}§a 余额为 §6{}",
                                    targetName,
                                    currencyName,
                                    balance.value()
                                ));
                            } else {
                                throw std::runtime_error("获取余额失败");
                            }
                        })) {
                        auto player = validatePlayer();
                        if (player) player->sendMessage("§c查询余额操作失败，请检查命令参数");
                    }
                    break;

                case CommandAdminOperation::his:
                    if (!handleTargetedOperation([](Player*            player,
                                                    const std::string& targetXuid,
                                                    int /*amount*/,
                                                    const std::string& targetName,
                                                    const std::string& currencyId) {
                            auto history = EconomyManager::getInstance().getPlayerTransactions(targetXuid, currencyId);
                            if (history.empty()) {
                                player->sendMessage(fmt::format("§e{} 暂无交易记录", targetName));
                                return;
                            }

                            const auto& config     = MoneyConfig::get();
                            auto        currencyIt = config.currencies.find(currencyId);
                            std::string currencyName =
                                currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                            player->sendMessage(fmt::format("§b{}§a 的 §b{}§a 交易记录：", targetName, currencyName));
                            for (const auto& record : history) {
                                player->sendMessage(fmt::format(
                                    "§7- {}，金额为 §6{}§7，余额为 §6{}",
                                    record.description,
                                    record.amount,
                                    record.balance
                                ));
                            }
                        })) {
                        auto player = validatePlayer();
                        if (player) player->sendMessage("§c查询交易记录操作失败，请检查命令参数");
                    }
                    break;

                case CommandAdminOperation::top: {
                    auto player = validatePlayer();
                    if (!player) break;

                    auto topPlayers = EconomyManager::getInstance().getTopBalanceList(currencyId, 10);
                    if (topPlayers.empty()) {
                        player->sendMessage("§e没有找到任何玩家数据");
                        break;
                    }

                    const auto& config     = MoneyConfig::get();
                    auto        currencyIt = config.currencies.find(currencyId);
                    std::string currencyName =
                        currencyIt != config.currencies.end() ? currencyIt->second.name : currencyId;
                    player->sendMessage(fmt::format("§b{} §a排行榜前10名：", currencyName));
                    int rank = 1;
                    for (const auto& entry : topPlayers) {
                        auto xuid       = entry.xuid;
                        auto balance    = entry.balance;
                        auto playerName = LeviLaminaAPI::getPlayerNameByXuid(xuid);
                        player->sendMessage(fmt::format("§e{}§7. §b{}§7 - §6{}", rank++, playerName, balance));
                    }
                    break;
                }

                case CommandAdminOperation::setinitial: {
                    auto player = validatePlayer();
                    if (!player) break;

                    if (param.Amount < 0) {
                        output.error("§c初始金额不能为负数");
                        break;
                    }

                    MoneyConfig::setInitialBalance(param.Amount);
                    player->sendMessage(fmt::format("§a成功设置初始金额为 §6{} 金币", param.Amount));
                    break;
                }

                case CommandAdminOperation::getinitial: {
                    auto player = validatePlayer();
                    if (!player) break;

                    auto initialBalance = MoneyConfig::getInitialBalance();
                    player->sendMessage(fmt::format("§a当前初始金额为 §6{} 金币", initialBalance));
                    break;
                }

                case CommandAdminOperation::reload: {
                    auto player = validatePlayer();
                    if (!player) break;

                    try {
                        MoneyConfig::reload();
                        // 同步配置文件中的币种到数据库（新增币种会被创建，已有币种的信息会被更新）
                        if (EconomyManager::getInstance().syncCurrenciesFromConfig()) {
                            player->sendMessage("§a配置已重新加载并同步到数据库");
                        } else {
                            player->sendMessage("§e配置已重新加载，但同步到数据库失败");
                        }
                    } catch (const ConfigException& e) {
                        output.error(fmt::format("重新加载配置失败：{}", e.what()));
                        if (player) {
                            player->sendMessage(fmt::format("§c配置重载失败：{}", e.what()));
                        }
                    }
                    break;
                }

                default:
                    output.error("未知的管理员操作");
                    break;
                }
            } catch (const std::exception& e) {
                output.error(fmt::format("操作失败：{}", e.what()));
            }
        });

    opCommand.overload<CurrencyCommand>()
        .required("Operation")
        .optional("CurrencyId")
        .optional("Param1")
        .optional("Param2")
        .optional("Param3")
        .execute([](CommandOrigin const& origin, CommandOutput& output, CurrencyCommand const& param, Command const&) {
            // 验证玩家权限
            auto actor = origin.getEntity();
            if (actor == nullptr || !actor->isType(ActorType::Player)) {
                output.error("只有玩家可以执行币种管理操作");
                return;
            }
            auto player = static_cast<Player*>(actor);
            if (!player->isOperator()) {
                output.error("你没有权限执行币种管理操作");
                return;
            }

            const auto& config = MoneyConfig::get();

            try {
                switch (param.Operation) {
                case CommandCurrencyOperation::list: {
                    if (config.currencies.empty()) {
                        player->sendMessage("§e没有配置任何币种");
                        break;
                    }
                    player->sendMessage("§a所有币种列表：");
                    for (const auto& [id, currency] : config.currencies) {
                        player->sendMessage(fmt::format(
                            "§7- §b{} §7(§e{}§7): §6{} §7- {}",
                            id,
                            currency.name,
                            currency.symbol,
                            currency.enabled ? "启用" : "禁用"
                        ));
                    }
                    break;
                }

                case CommandCurrencyOperation::info: {
                    std::string currencyId = param.CurrencyId.mText;
                    if (currencyId.empty()) {
                        output.error("请指定币种ID");
                        break;
                    }
                    auto currencyIt = config.currencies.find(currencyId);
                    if (currencyIt == config.currencies.end()) {
                        output.error(fmt::format("币种 {} 不存在", currencyId));
                        break;
                    }
                    const auto& currency = currencyIt->second;
                    player->sendMessage(fmt::format("§a币种信息：§b{}", currencyId));
                    player->sendMessage(fmt::format("§7- 名称: §b{}", currency.name));
                    player->sendMessage(fmt::format("§7- 符号: §6{}", currency.symbol));
                    player->sendMessage(fmt::format("§7- 显示格式: §f{}", currency.displayFormat));
                    player->sendMessage(fmt::format("§7- 状态: {}", currency.enabled ? "§a启用" : "§c禁用"));
                    player->sendMessage(fmt::format("§7- 初始余额: §6{}", currency.initialBalance));
                    player->sendMessage(fmt::format("§7- 最大余额: §6{}", currency.maxBalance));
                    player->sendMessage(fmt::format("§7- 最小转账金额: §6{}", currency.minTransferAmount));
                    player->sendMessage(fmt::format("§7- 转账手续费: §6{}", currency.transferFee));
                    player->sendMessage(fmt::format("§7- 手续费百分比: §6{}%", currency.feePercentage));
                    player->sendMessage(
                        fmt::format("§7- 允许玩家转账: {}", currency.allowPlayerTransfer ? "§a是" : "§c否")
                    );
                    break;
                }

                default:
                    output.error("币种管理功能尚未完全实现，请通过配置文件管理币种");
                    break;
                }
            } catch (const std::exception& e) {
                output.error(fmt::format("操作失败：{}", e.what()));
            }
        });
}
} // namespace rlx_money