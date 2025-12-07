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
    param([string]$buildDirectory)
    
    $windowsDir = Join-Path $buildDirectory "windows"
    $x64Dir = Join-Path $windowsDir "x64"
    $searchPaths = @(
        (Join-Path $x64Dir "release"),
        (Join-Path $x64Dir "debug"),
        $buildDirectory
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
    if (Test-Path $buildDirectory) {
        $libFile = Get-ChildItem -Path $buildDirectory -Filter "RLXMoney.lib" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
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
    
    # 使用 Join-Path 构建跨平台路径
    $srcModDir = Join-Path "src" "mod"
    $apiDir = Join-Path $srcModDir "api"
    $dataDir = Join-Path $srcModDir "data"
    $typesDir = Join-Path $srcModDir "types"
    
    $apiHeaders = @(
        (Join-Path $apiDir "RLXMoneyAPI.h"),
        (Join-Path $dataDir "DataStructures.h"),
        (Join-Path $typesDir "Types.h")
    )
    
    $copiedCount = 0
    foreach ($headerPath in $apiHeaders) {
        if (Test-Path $headerPath) {
            # 使用跨平台的路径分隔符进行替换
            $separator = [System.IO.Path]::DirectorySeparatorChar
            $srcModPath = "src" + $separator + "mod" + $separator
            $relPath = $headerPath -replace [regex]::Escape($srcModPath), ""
            
            $includeBase = Join-Path $targetBase "include"
            $target = Join-Path $includeBase $relPath
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
        [string]$libTargetName,
        [string]$outputDir
    )
    
    Write-Host "正在打包 $sdkName ..." -ForegroundColor Cyan
    
    $sdkDir = Join-Path $outputDir $sdkName
    $includeDir = Join-Path $sdkDir "include"
    if (-not (Test-Path $includeDir)) {
        New-Item -ItemType Directory -Path $includeDir -Force | Out-Null
    }
    
    # 复制库文件
    if ($libFile -and (Test-Path $libFile)) {
        $destPath = Join-Path $sdkDir $libTargetName
        Copy-Item -Path $libFile -Destination $destPath -Force
        Write-Host "  ✓ 复制库文件: $libTargetName" -ForegroundColor Green
    } else {
        Write-Host "  ⚠ 库文件未找到: $libFile" -ForegroundColor Yellow
        return $null
    }
    
    # 复制头文件
    $headerCount = Copy-SDKHeaders $sdkDir
    if ($headerCount -eq 0) {
        Write-Host "  ⚠ 警告：未复制任何头文件" -ForegroundColor Yellow
    } else {
        Write-Host "  ✓ 复制头文件: $headerCount 个文件" -ForegroundColor Green
    }
    
    return $sdkDir
}

# 跟踪打包结果
$packagedCount = 0
$failedPackages = @()

# 打包 SDK-shared（供其他插件链接到已安装的 RLXMoney.dll）
if ($SdkType -eq "all" -or $SdkType -eq "shared") {
    Write-Host ""
    $rlxmoneyLib = Find-RLXMoneyLib -buildDirectory $buildDir
    if (-not $rlxmoneyLib -or -not (Test-Path $rlxmoneyLib)) {
        Write-Host "警告：RLXMoney.lib 未找到" -ForegroundColor Yellow
        Write-Host "请确保已构建 RLXMoney 目标" -ForegroundColor Yellow
        $failedPackages += "sdk-shared"
    } else {
        Write-Host "找到 RLXMoney.lib: $rlxmoneyLib" -ForegroundColor Cyan
        $result = Package-SDK "sdk-shared" $rlxmoneyLib "RLXMoney.lib" $Output
        if ($result) {
            $packagedCount++
        } else {
            $failedPackages += "sdk-shared"
        }
    }
}

# 打包 SDK-static（供其他插件静态链接，不依赖 RLXMoney.dll）
if ($SdkType -eq "all" -or $SdkType -eq "static") {
    Write-Host ""
    $sdkStaticLib = Join-Path $libDir "SDK-static.lib"
    if (-not (Test-Path $sdkStaticLib)) {
        Write-Host "警告：SDK-static.lib 未找到: $sdkStaticLib" -ForegroundColor Yellow
        Write-Host "请确保已构建 SDK-static 目标" -ForegroundColor Yellow
        $failedPackages += "sdk-static"
    } else {
        $result = Package-SDK "sdk-static" $sdkStaticLib "SDK-static.lib" $Output
        if ($result) {
            $packagedCount++
        } else {
            $failedPackages += "sdk-static"
        }
    }
}

# 输出最终结果
Write-Host ""
if ($packagedCount -gt 0) {
    Write-Host "SDK 打包完成！成功打包 $packagedCount 个包，已创建在: $Output" -ForegroundColor Green
} else {
    Write-Host "错误：没有成功打包任何 SDK" -ForegroundColor Red
}

if ($failedPackages.Count -gt 0) {
    Write-Host "失败的包: $($failedPackages -join ', ')" -ForegroundColor Yellow
    exit 1
}

