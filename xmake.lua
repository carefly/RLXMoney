add_rules("mode.debug", "mode.release")

add_repositories("levimc-repo https://github.com/LiteLDev/xmake-repo.git")

-- add_requires("levilamina x.x.x") for a specific version
-- add_requires("levilamina develop") to use develop version
-- please note that you should add bdslibrary yourself if using dev version
if is_config("target_type", "server") then
    add_requires("levilamina 1.9.6", {configs = {target_type = "server"}})
else
    add_requires("levilamina 1.9.6", {configs = {target_type = "client"}})
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

-- ============================================================================
-- 公共配置和辅助函数
-- ============================================================================

-- 公共编译选项
local common_cxflags = {"/EHa", "/utf-8", "/W4", "/w44265", "/w44289", "/w44296", "/w45263", "/w44738", "/w45204"}
local common_defines = {"NOMINMAX", "UNICODE"}

-- 根据 target_type 添加平台宏定义（LeviLamina SDK 需要）
if is_config("target_type", "server") then
    table.insert(common_defines, "LL_PLAT_S")
else
    table.insert(common_defines, "LL_PLAT_C")
end

-- SDK 排除的文件模式（统一管理）
local sdk_excluded_files = {
    "src/mod/api/LeviLaminaAPI.*",
    "src/mod/commands/**",
    "src/mod/events/**",
    "src/mod/RLXMoney.*",
    "src/mod/MemoryOperators.*"
}

-- 应用公共配置到当前目标（在 target 块内调用）
function apply_common_config()
    add_cxflags(common_cxflags)
    add_defines(common_defines)
    set_exceptions("none")
    set_languages("c++20")
    set_symbols("none")
    -- 先从公共头查找，再从源码目录查找
    add_includedirs("include", "src")
end

-- 应用 SDK 配置到当前目标（在 target 块内调用）
function apply_sdk_config(defines, packages)
    apply_common_config()
    add_defines(defines)
    add_packages(packages)
    add_includedirs("src")
    -- 添加 mod 目录下的文件
    add_headerfiles("src/mod/**.h")
    add_files("src/mod/**.cpp")
    -- 排除插件特定的文件
    for _, pattern in ipairs(sdk_excluded_files) do
        remove_files(pattern)
    end
    set_targetdir("$(builddir)/lib")
end


-- ============================================================================
-- 构建目标
-- ============================================================================

-- 插件目标：完整的插件DLL（包含命令、事件监听等）
target("RLXMoney")
    add_rules("@levibuildscript/linkrule")
    add_rules("@levibuildscript/modpacker")
    apply_common_config()
    add_defines("RLXMONEY_EXPORTS")  -- DLL导出宏
    add_packages("levilamina", "sqlitecpp", "nlohmann_json")
    set_kind("shared")  -- 生成DLL和导入库
    add_headerfiles("src/**.h")
    add_files("src/**.cpp")
    add_includedirs("src")

-- SDK动态库目标：为其他插件提供链接到 RLXMoney.dll 的能力
-- 注意：此目标主要用于开发/测试，实际发布时直接使用 RLXMoney.lib（来自 RLXMoney.dll）
-- 其他插件链接 RLXMoney.lib，运行时使用已安装的 RLXMoney.dll
-- SDK-shared 不需要生成 DLL 和 .exp，只需要 .lib（实际使用 RLXMoney.lib）
target("SDK-shared")
    apply_sdk_config("RLXMONEY_EXPORTS", {"sqlitecpp", "nlohmann_json"})
    set_kind("shared")  -- 生成DLL和导入库（仅用于开发，发布时使用 RLXMoney.lib）
    -- 注意：发布时不需要 SDK-shared.dll 和 SDK-shared.exp，只使用 RLXMoney.lib

-- SDK静态库目标：为其他插件提供静态链接选项
-- 其他插件可以静态链接此库，不依赖 RLXMoney.dll
target("SDK-static")
    apply_sdk_config("RLXMONEY_STATIC", {"sqlitecpp", "nlohmann_json"})
    set_kind("static")  -- 生成静态库

-- 测试目标
target("tests")
    apply_common_config()
    add_defines("TESTING", "RLXMONEY_EXPORTS")
    add_packages("catch2", "sqlitecpp", "nlohmann_json")
    set_kind("binary")
    add_files("test/**.cpp")
    add_includedirs("src", "test", "test/stubs")

    -- 添加需要测试的源文件
    local test_source_files = {
        "src/mod/types/Types.cpp",
        "src/mod/exceptions/MoneyException.cpp",
        "test/mocks/MockLeviLaminaAPI.cpp",
        "test/utils/CommandTestHelper.cpp",
        "test/utils/TestTempManager.cpp",
        "src/mod/database/DatabaseManager.cpp",
        "src/mod/core/SystemInitializer.cpp",
        "src/mod/dao/PlayerDAO.cpp",
        "src/mod/dao/TransactionDAO.cpp",
        "src/mod/economy/EconomyManager.cpp",
        "src/mod/api/RLXMoneyAPI.cpp"
    }
    for _, file in ipairs(test_source_files) do
        add_files(file)
    end

    -- 注意：不添加 MemoryOperators.cpp，因为它依赖 LeviLamina 的内存操作符
    -- 注意：不添加 LeviLaminaAPI.cpp，因为在测试环境中不可用
    -- 注意：不添加 Commands.cpp 和 RLXMoney.cpp，因为它们依赖 LeviLamina 框架
    -- 注意：命令测试通过 CommandTestHelper 间接测试业务逻辑，不直接测试命令注册系统

-- ============================================================================
-- 自定义任务
-- ============================================================================

-- 注意：SDK 打包功能已迁移到 package-sdk.ps1 脚本
-- 使用方法：powershell -ExecutionPolicy Bypass -File package-sdk.ps1 -Output sdk-packages [-SdkType all|shared|static]
-- 原因：xmake 自定义任务在 GitHub Actions 环境中无法正常工作