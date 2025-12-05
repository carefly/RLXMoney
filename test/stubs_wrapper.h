#pragma once

#ifdef TESTING

// 这个文件确保在测试环境中，所有 LeviLamina 相关的头文件都使用 stub 版本
// 通过将 test/stubs 目录添加到包含路径的前面，编译器会优先使用 stub 版本

// 定义一些常用的类型别名，避免重复定义
#ifndef COMMAND_TYPES_DEFINED
#define COMMAND_TYPES_DEFINED

// 这些枚举在 Commands.h 中定义，但我们需要在测试环境中使用
// 由于我们不直接包含 Commands.h（它依赖 LeviLamina），我们在这里定义它们
namespace rlx_money {
    enum CommandBasicOperation : int { 
        query = 1, 
        history = 2 
    };
    
    enum CommandPayOperation : int { 
        pay = 1 
    };
    
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
}

#endif // COMMAND_TYPES_DEFINED

#endif // TESTING



