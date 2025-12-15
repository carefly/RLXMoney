-- RLXMoney 使用示例项目
-- 演示如何集成和使用 RLXMoney SDK

add_rules("mode.debug", "mode.release")

-- 添加 LeviLamina 依赖
add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")
if is_config("target_type", "server") then
    add_requires("levilamina 1.7.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.7.0", {configs = {target_type = "client"}})
end

-- 方式一：使用 xmake 包（推荐）
-- 需要先执行: xmake repo rlxmoney https://github.com/carefly/rlxmoney-xmake-repo
add_requires("rlxmoney v1.0.1", {configs = {shared = true}})

-- 方式二：使用本地 SDK（注释掉方式一，取消注释方式二）
-- add_includedirs("../RLXMoney/sdk-windows-x64-shared/include")
-- add_linkdirs("../RLXMoney/sdk-windows-x64-shared/lib")
-- add_links("RLXMoney")
-- add_defines("RLXMONEY_IMPORTS")

-- 方式三：使用静态库
-- add_requires("rlxmoney v1.0.0", {configs = {shared = false}})
-- 或
-- add_includedirs("../RLXMoney/sdk-windows-x64-static/include")
-- add_linkdirs("../RLXMoney/sdk-windows-x64-static/lib")
-- add_links("SDK-static")
-- add_defines("RLXMONEY_STATIC")

-- 示例插件目标
target("MyMoneyPlugin")
    -- 插件基础配置
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    set_kind("shared")
    set_languages("c++20")

    -- 编译配置
    add_cxflags("/EHa", "/utf-8", "/W4")
    add_defines("NOMINMAX", "UNICODE")
    set_exceptions("none")
    set_symbols("debug")

    -- 添加依赖
    add_packages("levilamina", "rlxmoney")

    -- 添加源文件
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")

-- 示例使用：
-- 在你的代码中：
-- #include "mod/api/RLXMoneyAPI.h"
--
-- void onPlayerJoin(Player* player) {
--     auto balance = rlx_money::RLXMoneyAPI::getBalance(player->getXuid(), "gold");
--     if (!balance.has_value()) {
--         // 新玩家，初始化余额
--         rlx_money::RLXMoneyAPI::setBalance(player->getXuid(), "gold", 1000);
--     }
-- }