#pragma once

#include <mc/server/commands/CommandRawText.h>

namespace rlx_money {

enum CommandBasicOperation : int { query = 1, history = 2 };
enum CommandPayOperation : int { pay = 1 };
enum CommandAdminOperation : int {
    set        = 1,
    give       = 2,
    take       = 3,
    check      = 4,
    his        = 5,
    top        = 6,
    setinitial = 7,
    getinitial = 8,
    reload     = 9
};

enum CommandCurrencyOperation : int {
    list = 1,
    create = 2,
    delete_ = 3,
    enable = 4,
    disable = 5,
    config = 6,
    info = 7
};

struct BasicCommand {
    CommandBasicOperation Operation{static_cast<CommandBasicOperation>(0)};
    CommandRawText        Currency{""};  // 可选币种参数
};
struct PayCommand {
    CommandPayOperation Operation{static_cast<CommandPayOperation>(0)};
    CommandRawText      Target{""};
    int                 Amount{0};
    CommandRawText      Currency{""};  // 可选币种参数
};
struct AdminCommand {
    CommandAdminOperation Operation{static_cast<CommandAdminOperation>(0)};
    CommandRawText        Target{""};
    int                   Amount{0};
    CommandRawText        Currency{""};  // 可选币种参数
};
struct CurrencyCommand {
    CommandCurrencyOperation Operation{static_cast<CommandCurrencyOperation>(0)};
    CommandRawText           CurrencyId{""};
    CommandRawText           Param1{""};  // 用于create、config等需要额外参数的命令
    CommandRawText           Param2{""};
    CommandRawText           Param3{""};
    int                      IntParam{0};
};

class Commands {
public:
    static void registerCommands();
};

} // namespace rlx_money