# RLXMoney

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/carefly/RLXMoney)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![LeviLamina](https://img.shields.io/badge/LeviLamina-compatible-green.svg)](https://github.com/LiteLDev/LeviLamina)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)

基于LeviLamina框架的Minecraft服务器经济系统插件，提供完整的虚拟货币系统和交易管理功能。采用分层架构设计，具备高性能、高可靠性和良好的扩展性。

## 📋 功能特性

### 💰 核心经济功能
- **多币种支持**: 灵活的货币配置系统，支持不同币种独立管理
- **玩家金钱管理**: 余额查询、设置、增加、扣除
- **玩家间转账**: 支持玩家之间互相转账（可配置手续费）
- **交易记录**: 完整的交易历史记录和查询功能
- **财富排行榜**: 实时更新的财富排行榜
- **新玩家初始化**: 自动给新玩家发放初始金额

### 🛠️ 管理功能
- **配置热重载**: 支持运行时重新加载配置文件
- **动态初始金额**: 管理员可动态修改新玩家初始金额
- **数据持久化**: SQLite数据库存储，支持WAL模式优化


## 💬 命令系统

### 普通玩家命令

| 命令                                | 权限要求 | 说明                                           | 示例                                                  |
| ----------------------------------- | -------- | ---------------------------------------------- | ----------------------------------------------------- |
| `/money query [币种]`               | 无       | 查询自己当前余额（不指定币种则显示所有币种）   | `/money query` 或 `/money query gold`                 |
| `/money history [币种]`             | 无       | 查看自己的交易记录（不指定币种则显示所有币种） | `/money history` 或 `/money history gold`             |
| `/money pay <玩家名> <金额> [币种]` | 无       | 给其他玩家转账（不指定币种使用默认币种）       | `/money pay Steve 100` 或 `/money pay Steve 100 gold` |

### 管理员命令（仅限OP）

| 命令                                   | 说明                         | 示例                       |
| -------------------------------------- | ---------------------------- | -------------------------- |
| `/moneyop set <玩家名> <金额> [币种]`  | 设置玩家余额                 | `/moneyop set Steve 5000`  |
| `/moneyop give <玩家名> <金额> [币种]` | 给玩家增加金钱               | `/moneyop give Steve 100`  |
| `/moneyop take <玩家名> <金额> [币种]` | 扣除玩家金钱                 | `/moneyop take Steve 50`   |
| `/moneyop check <玩家名> [币种]`       | 查看指定玩家余额             | `/moneyop check Steve`     |
| `/moneyop his <玩家名> [币种]`         | 查看指定玩家交易记录         | `/moneyop his Steve`       |
| `/moneyop top [币种]`                  | 查看财富排行榜               | `/moneyop top`             |
| `/moneyop setinitial <金额>`           | 设置默认币种的新玩家初始金额 | `/moneyop setinitial 2000` |
| `/moneyop getinitial`                  | 查看默认币种的当前初始金额   | `/moneyop getinitial`      |
| `/moneyop reload`                      | 重新加载配置文件             | `/moneyop reload`          |

### 币种管理命令（仅限OP）

| 命令                          | 说明         | 示例                          |
| ----------------------------- | ------------ | ----------------------------- |
| `/moneyop currency list`      | 查看所有币种 | `/moneyop currency list`      |
| `/moneyop currency info <ID>` | 查看币种详情 | `/moneyop currency info gold` |

### 权限说明

- **普通玩家**: 可使用所有 `/money` 开头的命令进行基础经济操作
- **管理员（OP）**: 可使用 `/moneyop` 开头的命令进行服务器管理操作
- 权限检查基于 LeviLamina 的 OP 系统，无需额外配置权限节点

## ⚙️ 配置文件

配置文件位于 `plugins/RLXModeResources/data/money/config.json`：

```json
{
    "database": {
        "path": "plugins/RLXModeResources/data/money/money.db",
        "optimization": {
            "wal_mode": true,
            "cache_size": 2000,
            "synchronous": "NORMAL",
            "temp_store": "memory"
        }
    },
    "currencies": {
        "gold": {
            "name": "金币",
            "symbol": "§6",
            "enabled": true,
            "initialBalance": 1000,
            "maxBalance": 1000000000,
            "minTransferAmount": 1,
            "transferFee": 0,
            "feePercentage": 0.0,
            "allowPlayerTransfer": true,
            "displayFormat": "§6{}{}"
        },
        "diamond": {
            "name": "钻石",
            "symbol": "§b",
            "enabled": true,
            "initialBalance": 100,
            "maxBalance": 100000000,
            "minTransferAmount": 1,
            "transferFee": 1,
            "feePercentage": 2.0,
            "allowPlayerTransfer": true,
            "displayFormat": "§b{}{}"
        }
    },
    "default_currency": "gold",
    "top_list": {
        "default_count": 10,
        "max_count": 50,
        "update_interval": 300
    },
    "logging": {
        "enable_debug": false,
        "log_transactions": true,
        "log_retention_days": 30
    }
}
```

### 配置项说明

#### 数据库配置
- **path**: SQLite数据库文件路径
- **wal_mode**: WAL模式，提高并发性能
- **cache_size**: 缓存大小（页面数）
- **synchronous**: 同步模式（OFF/NORMAL/FULL）
- **temp_store**: 临时存储位置

#### 币种配置
每个币种支持以下配置：
- **name**: 币种显示名称
- **symbol**: 币种符号（支持颜色代码）
- **enabled**: 是否启用该币种
- **initialBalance**: 新玩家该币种初始余额
- **maxBalance**: 最大余额限制（0表示无限制）
- **minTransferAmount**: 最小转账金额
- **transferFee**: 固定转账手续费
- **feePercentage**: 百分比手续费（0.0-100.0）
- **allowPlayerTransfer**: 是否允许玩家间转账
- **displayFormat**: 显示格式（第一个{}为符号，第二个{}为金额）

#### 全局配置
- **default_currency**: 默认币种ID

#### 排行榜配置
- **default_count**: 默认显示数量
- **max_count**: 最大显示数量
- **update_interval**: 更新间隔（秒）

#### 日志配置
- **enable_debug**: 是否启用调试日志
- **log_transactions**: 是否记录交易日志
- **log_retention_days**: 日志保留天数

## 🔐 权限系统

RLXMoney 使用基于 LeviLamina 框架的简化权限系统：

### 权限实现

- **普通玩家命令** (`/money`)：所有玩家均可使用，包括余额查询、转账、交易记录查看
- **管理员命令** (`/moneyop`)：仅限服务器管理员（OP）使用，包括玩家管理、配置管理、币种管理

### 权限检查机制

插件使用 LeviLamina 内置的 OP 系统进行权限验证：
- 普通玩家命令无需任何特殊权限
- 管理员命令要求玩家具有 OP 权限（`isOperator()` 返回 true）
- 无需额外配置权限节点或安装权限插件

## 🔌 插件集成

RLXMoney 提供完整的 API 接口供其他插件调用：

```cpp
// 基础操作
rlx_money::RLXMoneyAPI::getBalance(playerXuid, currencyId)           // 获取余额
rlx_money::RLXMoneyAPI::getAllBalances(playerXuid)                    // 获取所有币种余额
rlx_money::RLXMoneyAPI::setBalance(playerXuid, currencyId, amount)     // 设置余额
rlx_money::RLXMoneyAPI::addMoney(playerXuid, currencyId, amount)       // 增加金钱
rlx_money::RLXMoneyAPI::reduceMoney(playerXuid, currencyId, amount)    // 扣除金钱
rlx_money::RLXMoneyAPI::transferMoney(fromXuid, toXuid, currencyId, amount) // 转账

// 查询操作
rlx_money::RLXMoneyAPI::playerExists(playerXuid)                      // 检查玩家是否存在
rlx_money::RLXMoneyAPI::hasSufficientBalance(playerXuid, currencyId, amount) // 检查余额是否充足
rlx_money::RLXMoneyAPI::getTopBalanceList(currencyId, limit)           // 获取财富排行榜
rlx_money::RLXMoneyAPI::getPlayerTransactions(playerXuid, currencyId, page, pageSize) // 获取交易历史
rlx_money::RLXMoneyAPI::getPlayerTransactionCount(playerXuid)          // 获取交易记录总数

// 币种和统计
rlx_money::RLXMoneyAPI::getEnabledCurrencyIds()                        // 获取所有启用的币种ID
rlx_money::RLXMoneyAPI::getDefaultCurrencyId()                         // 获取默认币种ID
rlx_money::RLXMoneyAPI::getTotalWealth(currencyId)                     // 获取服务器总财富
rlx_money::RLXMoneyAPI::getPlayerCount()                                // 获取玩家总数
rlx_money::RLXMoneyAPI::isValidAmount(amount)                          // 验证金额是否有效
```

## 💾 数据存储

RLXMoney 使用 SQLite 数据库安全存储所有经济数据：

- **事务保护**: 所有操作都通过事务确保数据完整性，失败时自动回滚
- **高性能**: 使用 WAL 模式优化并发访问
- **数据持久化**: 所有经济数据自动保存到 SQLite 数据库
- **手动备份**: 建议定期手动备份数据库文件（`money.db`）

## 🚀 部署指南

### 安装步骤

1. **下载插件**
   ```bash
   # 从 Releases 下载对应版本
   wget https://github.com/carefly/RLXMoney/releases/latest/download/RLXMoney.dll
   ```

2. **放置文件**
   ```
   server/
   ├── plugins/
   │   └── RLXMoney.dll
   └── RLXModeResources/
       └── data/
           └── money/
               ├── config.json
               └── money.db
   ```

3. **配置权限**
   无需额外配置权限节点，插件使用 LeviLamina 内置的 OP 系统进行权限验证

4. **重启服务器**
   插件会自动初始化数据库和配置

### 升级指南

1. 备份现有数据（复制 `money.db` 文件）
2. 停止服务器
3. 替换插件文件
4. 重启服务器（数据库结构会自动创建，兼容现有数据）

### 备份策略

- 定期备份 `money.db` 文件
- 保留 `config.json` 配置文件
- 建议使用版本控制系统跟踪配置变更

## 📄 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

## 🆘 支持

### 获取帮助

- **问题反馈**: [GitHub Issues](https://github.com/carefly/RLXMoney/issues)
- **讨论交流**: [GitHub Discussions](https://github.com/carefly/RLXMoney/discussions)

### 常见问题

<details>
<summary>插件无法加载？</summary>

检查以下项目：
1. LeviLamina 版本是否兼容
2. 插件文件是否放置在正确位置
3. 查看服务器日志中的错误信息
</details>

<details>
<summary>数据库连接失败？</summary>

确认：
1. 数据库文件路径是否正确
2. 目录权限是否足够
3. 磁盘空间是否充足
</details>

<details>
<summary>命令无法使用？</summary>

检查：
1. 权限配置是否正确
2. 命令格式是否符合要求
3. 玩家是否在线（部分命令需要）
</details>

---

**⭐ 如果这个项目对你有帮助，请给我们一个 Star！**
