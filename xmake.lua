add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

-- add_requires("levilamina x.x.x") for a specific version
-- add_requires("levilamina develop") to use develop version
-- please note that you should add bdslibrary yourself if using dev version
if is_config("target_type", "server") then
    add_requires("levilamina 1.7.0", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.7.0", {configs = {target_type = "client"}})
end

add_requires("levibuildscript")
-- 添加SQLite和JSON依赖
add_requires("sqlitecpp")
add_requires("nlohmann_json")
-- 添加测试依赖
add_requires("catch2")

if not has_config("vs_runtime") then
    set_runtimes("MD")
end

option("target_type")
    set_default("server")
    set_showmenu(true)
    set_values("server", "client")
option_end()


target("RLXMoney") -- Change this to your mod name.
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE", "RLXMONEY_EXPORTS")
    add_packages("levilamina", "sqlitecpp", "nlohmann_json")
    set_exceptions("none") -- To avoid conflicts with /EHa.
    set_kind("shared")
    set_languages("c++20")
    set_symbols("debug")
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")

    -- if is_config("target_type", "server") then
    --     add_includedirs("src-server")
    --     add_files("src-server/**.cpp")
    -- else
    --     add_includedirs("src-client")
    --     add_files("src-client/**.cpp")
    -- end

-- 测试目标
target("RLXMoney_tests")
    add_cxflags( "/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204")
    add_defines("NOMINMAX", "UNICODE", "TESTING", "RLXMONEY_EXPORTS")  -- 添加TESTING宏定义
    add_packages("catch2", "sqlitecpp", "nlohmann_json")
    set_exceptions("none")
    set_kind("binary")
    set_languages("c++20")
    add_files("test/**.cpp")
    add_includedirs("src", "test", "test/stubs")
    set_symbols("debug")

    -- 添加需要测试的源文件
    add_files("src/mod/types/Types.cpp")
    add_files("src/mod/exceptions/MoneyException.cpp")
    add_files("test/mocks/MockLeviLaminaAPI.cpp")
    add_files("test/utils/CommandTestHelper.cpp")
    add_files("test/utils/TestTempManager.cpp")

    -- 添加核心功能文件用于测试
    add_files("src/mod/config/ConfigManager.cpp")
    add_files("src/mod/database/DatabaseManager.cpp")
    add_files("src/mod/core/SystemInitializer.cpp")
    add_files("src/mod/dao/PlayerDAO.cpp")
    add_files("src/mod/dao/TransactionDAO.cpp")
    add_files("src/mod/economy/EconomyManager.cpp")
    add_files("src/mod/api/RLXMoneyAPI.cpp")

    -- 注意：不添加 MemoryOperators.cpp，因为它依赖 LeviLamina 的内存操作符
    -- 注意：不添加 LeviLaminaAPI.cpp，因为在测试环境中不可用
    -- 注意：不添加 Commands.cpp 和 RLXMoney.cpp，因为它们依赖 LeviLamina 框架
    -- 注意：命令测试通过 CommandTestHelper 间接测试业务逻辑，不直接测试命令注册系统