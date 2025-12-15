package("rlxmoney")
    -- 设置包的基本信息
    set_homepage("https://github.com/carefly/RLXMoney")
    set_description("A comprehensive money management plugin for Minecraft based on LeviLamina")
    set_license("MIT")

    -- 维护者信息
    set_maintainer("RLX Team")

    -- 支持的版本
    set_versions("1.0.1")

    -- 分类
    set_categories("plugin", "minecraft", "levilamina", "economy")

    -- 支持的平台
    set_allowed_plats("windows")
    set_allowed_archs("x64", "arm64")

    -- 包配置选项
    add_configs("shared", {description = "Use shared library (DLL) instead of static library", default = true, type = "boolean"})
    add_configs("runtime", {description = "Link runtime statically", default = false, type = "boolean"})
    add_configs("debug", {description = "Enable debug symbols", default = false, type = "boolean"})

    -- 下载 URL（支持不同版本和配置）
    on_load(function(package)
        -- 根据配置确定包名
        if package:config("shared") then
            package:add("defines", "RLXMONEY_IMPORTS")
            package:set("basename", "rlxmoney")
        else
            package:add("defines", "RLXMONEY_STATIC")
            package:set("basename", "rlxmoney-static")
        end

        -- 添加包含目录
        package:add("includedirs", "include")

        -- 添加库目录和链接库
        -- 库文件会被安装到 lib 目录
        package:add("linkdirs", "lib")
        if package:config("shared") then
            package:add("links", "RLXMoney")
        else
            package:add("links", "SDK-static")
        end

        -- 添加依赖
        package:add("deps", "sqlitecpp", "nlohmann_json")

        -- 设置运行时库
        if package:config("runtime") then
            package:set("runtimes", "MT")
        else
            package:set("runtimes", "MD")
        end

        -- 添加编译定义
        package:add("defines", "NOMINMAX", "UNICODE")
        package:add("cxflags", "/EHa", "/utf-8", "/W4")
    end)

    -- 安装函数
    on_install(function(package)
        -- 根据版本和配置下载对应的包
        local version = package:version()
        local config = package:config("shared") and "shared" or "static"
        local arch = package:is_arch("x64") and "x64" or "arm64"
        local debug_suffix = package:config("debug") and "-debug" or ""

        -- 构造下载 URL
        local urls = {
            string.format("https://github.com/carefly/RLXMoney/releases/download/v%s/sdk-windows-%s-%s%s.zip",
                         version, arch, config, debug_suffix)
        }
        package:set("urls", urls)

        -- 验证文件哈希（可选）
        -- package:set("sha256", "...")

        -- 解压安装
        package:extract()

        -- 复制头文件到安装目录
        -- 如果解压后有顶层目录（如 sdk-windows-x64-shared），extract() 会自动进入
        if os.isdir("include") then
            os.cp("include/*", package:installdir("include"))
        end

        -- 复制库文件到安装目录（库文件在 SDK 包的根目录）
        -- 动态库使用 RLXMoney.lib（导入库），静态库使用 SDK-static.lib
        local lib_name = package:config("shared") and "RLXMoney.lib" or "SDK-static.lib"
        
        if os.isfile(lib_name) then
            os.cp(lib_name, package:installdir("lib"))
        end

        -- 注意：shared 版本的 DLL 文件应该已经随 RLXMoney 插件安装，
        -- SDK 包只包含导入库（.lib）或静态库，不包含运行时 DLL
    end)

    -- 测试函数
    on_test(function(package)
        -- 测试头文件是否存在（include 目录已在 includedirs 中，所以使用相对路径）
        assert(package:has_cxfuncs("rlx_money::RLXMoneyAPI::getBalance", {includes = {"mod/api/RLXMoneyAPI.h"}}))
    end)