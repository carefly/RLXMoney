# 命令测试方案说明

## 概述

本方案提供了一个**低成本的命令测试解决方案**，**不需要修改源代码**即可测试命令功能。

## 方案设计

### 核心思路

由于 `Commands.cpp` 中的命令逻辑封装在 lambda 函数中，且依赖 LeviLamina 框架，无法直接访问。本方案采用**间接测试**的方式：

1. **不直接测试命令注册和执行流程**
2. **测试命令背后的业务逻辑**（EconomyManager、ConfigManager 等）
3. **验证命令应该产生的效果**（余额变化、交易记录等）

### 文件结构

```
test/
├── mocks/
│   ├── MockLeviLaminaAPI.h/cpp    # Mock 玩家 API（已存在）
│   └── MockCommandTypes.h          # Mock 命令类型（新增）
├── utils/
│   └── CommandTestHelper.h/cpp     # 命令测试辅助类（新增）
└── test_commands.cpp                # 命令测试用例（新增）
```

## 使用方法

### 1. 测试基本命令

```cpp
// 测试查询余额命令
REQUIRE(rlx_money::test::CommandTestHelper::testBasicQueryCommand(
    "player_xuid",
    "player_name",
    "gold",  // 币种ID，空字符串表示查询所有币种
    true     // 期望成功
));
```

### 2. 测试转账命令

```cpp
// 测试转账命令
REQUIRE(rlx_money::test::CommandTestHelper::testPayCommand(
    "from_xuid",
    "from_name",
    "to_name",
    100,     // 转账金额
    "gold",  // 币种ID
    true     // 期望成功
));
```

### 3. 测试管理员命令

```cpp
// 测试设置余额命令
REQUIRE(rlx_money::test::CommandTestHelper::testAdminSetCommand(
    "admin_xuid",
    "admin_name",
    "target_name",
    5000,    // 设置金额
    "gold",  // 币种ID
    true     // 期望成功
));
```

## 测试覆盖范围

### 已实现的测试

- ✅ BasicCommand - 查询余额
- ✅ BasicCommand - 查询历史记录
- ✅ PayCommand - 转账操作
- ✅ AdminCommand - 设置余额
- ✅ AdminCommand - 给予金钱
- ✅ AdminCommand - 扣除金钱
- ✅ AdminCommand - 查询余额
- ✅ AdminCommand - 查询历史记录
- ✅ AdminCommand - 排行榜
- ✅ AdminCommand - 设置初始金额
- ✅ AdminCommand - 获取初始金额
- ✅ AdminCommand - 重载配置
- ✅ CurrencyCommand - 币种列表
- ✅ CurrencyCommand - 币种信息

## 优势

1. **无需修改源代码** - 所有测试代码都在 `test/` 目录下
2. **低成本** - 复用现有的 Mock 基础设施
3. **测试业务逻辑** - 验证命令应该产生的实际效果
4. **易于扩展** - 可以轻松添加新的测试用例

## 技术实现

### Stub 头文件

为了避免测试工程依赖 LeviLamina 框架，我们创建了完整的 stub 头文件系统：

- `test/stubs/mc/server/commands/` - 命令相关的 stub
- `test/stubs/mc/world/actor/` - Actor 相关的 stub
- `test/stubs/ll/api/command/` - LeviLamina 命令 API 的 stub

这些 stub 文件在测试环境中替代真实的 LeviLamina 头文件，确保测试工程可以独立编译。

### 构建配置

在 `xmake.lua` 中，测试目标配置了：
- `add_includedirs("test/stubs")` - 将 stub 目录添加到包含路径
- `add_defines("TESTING")` - 定义 TESTING 宏，启用 stub 版本

## 限制

1. **不测试命令注册** - 不测试命令是否成功注册到 LeviLamina
2. **不测试命令参数解析** - 不测试 LeviLamina 的命令参数解析
3. **不测试权限检查** - 不测试 LeviLamina 的权限系统
4. **间接测试** - 通过业务逻辑验证命令效果，而非直接执行命令
5. **不依赖 LeviLamina** - 测试工程完全独立，不包含任何 LeviLamina 依赖

## 运行测试

```bash
# 构建测试
xmake build tests

# 运行测试
xmake run tests
```

## 扩展测试

要添加新的命令测试，只需：

1. 在 `CommandTestHelper` 中添加新的测试方法
2. 在 `test_commands.cpp` 中添加新的测试用例
3. 使用现有的 Mock 基础设施

示例：

```cpp
// 在 CommandTestHelper.h 中添加
static bool testNewCommand(...);

// 在 CommandTestHelper.cpp 中实现
bool CommandTestHelper::testNewCommand(...) {
    // 测试逻辑
}

// 在 test_commands.cpp 中使用
TEST_CASE("新命令测试", "[commands][new]") {
    REQUIRE(rlx_money::test::CommandTestHelper::testNewCommand(...));
}
```

