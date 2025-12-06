# SDK 打包脚本
# 用途：为其他插件准备 SDK 包（库文件 + 头文件）
# 用法: .\package-sdk.ps1 [-Output sdk-packages] [-SdkType all|shared|static]
# 参数:
#   -Output: 指定输出目录（默认：sdk-packages）
#   -SdkType: 指定打包类型（默认：all，可选：all|shared|static）

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [string]$Output = "sdk-packages",
    
    [Parameter(Mandatory=$false)]
    [ValidateSet("all", "shared", "static")]
    [string]$SdkType = "all"
)

# 设置错误处理：遇到错误立即退出
$ErrorActionPreference = "Stop"

# 设置控制台输出编码为 UTF-8
[Console]::OutputEncoding = [System.Text.Encoding]::UTF8
$OutputEncoding = [System.Text.Encoding]::UTF8

Write-Host "正在打包 SDK 到: $Output" -ForegroundColor Green

# 获取构建目录
$buildDir = "build"
$libDir = Join-Path $buildDir "lib"

# 查找 RLXMoney.lib 的辅助函数（在 build 目录中递归搜索）
function Find-RLXMoneyLib {
    $searchPaths = @(
        Join-Path (Join-Path (Join-Path $buildDir "windows") "x64") "release",
        Join-Path (Join-Path (Join-Path $buildDir "windows") "x64") "debug",
        $buildDir
    )
    
    foreach ($searchPath in $searchPaths) {
        if (Test-Path $searchPath) {
            $libFile = Get-ChildItem -Path $searchPath -Filter "RLXMoney.lib" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
            if ($libFile) {
                return $libFile.FullName
            }
        }
    }
    
    # 如果上述路径都没找到，在整个 build 目录中搜索
    if (Test-Path $buildDir) {
        $libFile = Get-ChildItem -Path $buildDir -Filter "RLXMoney.lib" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($libFile) {
            return $libFile.FullName
        }
    }
    
    return $null
}

# 检查构建目录是否存在
if (-not (Test-Path $buildDir)) {
    Write-Host "错误：构建目录不存在: $buildDir" -ForegroundColor Red
    Write-Host "请先运行 xmake 构建项目" -ForegroundColor Yellow
    exit 1
}

# 创建输出目录
if (-not (Test-Path $Output)) {
    New-Item -ItemType Directory -Path $Output -Force | Out-Null
    Write-Host "已创建输出目录: $Output" -ForegroundColor Cyan
}

# 复制 SDK API 头文件及其依赖
function Copy-SDKHeaders {
    param([string]$targetBase)
    
    $apiHeaders = @(
        "src\mod\api\RLXMoneyAPI.h",
        "src\mod\data\DataStructures.h",
        "src\mod\types\Types.h"
    )
    
    $copiedCount = 0
    foreach ($headerPath in $apiHeaders) {
        if (Test-Path $headerPath) {
            $relPath = $headerPath -replace "^src\\mod\\", ""
            $target = Join-Path $targetBase "include\$relPath"
            $targetDir = Split-Path $target -Parent
            if (-not (Test-Path $targetDir)) {
                New-Item -ItemType Directory -Path $targetDir -Force | Out-Null
            }
            Copy-Item -Path $headerPath -Destination $target -Force
            $copiedCount++
            Write-Host "  ✓ 复制头文件: $relPath" -ForegroundColor Green
        } else {
            Write-Host "  ⚠ 头文件未找到: $headerPath" -ForegroundColor Yellow
        }
    }
    return $copiedCount
}

# 打包单个 SDK 的通用函数
function Package-SDK {
    param(
        [string]$sdkName,
        [string]$libFile,
        [string]$libTargetName
    )
    
    Write-Host "正在打包 $sdkName ..." -ForegroundColor Cyan
    
    $sdkDir = Join-Path $Output $sdkName
    $includeDir = Join-Path $sdkDir "include"
    if (-not (Test-Path $includeDir)) {
        New-Item -ItemType Directory -Path $includeDir -Force | Out-Null
    }
    
    # 复制库文件
    if ($libFile -and (Test-Path $libFile)) {
        Copy-Item -Path $libFile -Destination (Join-Path $sdkDir $libTargetName) -Force
        Write-Host "  ✓ 复制库文件: $libTargetName" -ForegroundColor Green
    } else {
        Write-Host "  ⚠ 库文件未找到: $libFile" -ForegroundColor Yellow
    }
    
    # 复制头文件
    $headerCount = Copy-SDKHeaders $sdkDir
    Write-Host "  ✓ 复制头文件: $headerCount 个文件" -ForegroundColor Green
    
    return $sdkDir
}

# 打包 SDK-shared（供其他插件链接到已安装的 RLXMoney.dll）
if ($SdkType -eq "all" -or $SdkType -eq "shared") {
    $rlxmoneyLib = Find-RLXMoneyLib
    if (-not $rlxmoneyLib -or -not (Test-Path $rlxmoneyLib)) {
        Write-Host "警告：RLXMoney.lib 未找到" -ForegroundColor Yellow
        Write-Host "请确保已构建 RLXMoney 目标" -ForegroundColor Yellow
    } else {
        Write-Host "找到 RLXMoney.lib: $rlxmoneyLib" -ForegroundColor Cyan
        Package-SDK "sdk-shared" $rlxmoneyLib "RLXMoney.lib"
    }
}

# 打包 SDK-static（供其他插件静态链接，不依赖 RLXMoney.dll）
if ($SdkType -eq "all" -or $SdkType -eq "static") {
    $sdkStaticLib = Join-Path $libDir "SDK-static.lib"
    if (-not (Test-Path $sdkStaticLib)) {
        Write-Host "警告：SDK-static.lib 未找到: $sdkStaticLib" -ForegroundColor Yellow
        Write-Host "请确保已构建 SDK-static 目标" -ForegroundColor Yellow
    } else {
        Package-SDK "sdk-static" $sdkStaticLib "SDK-static.lib"
    }
}

Write-Host "SDK 打包完成！包已创建在: $Output" -ForegroundColor Green

