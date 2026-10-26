# SPDX-License-Identifier: Apache-2.0
#
# 平台工作区构建环境加载脚本（需 dot-source 方式加载）:
#
#   . .\scripts\env.ps1
#
# 加载内容:
#   1. 工作区 .venv（Python 3.12，内含 west / cmake）
#   2. dtc / gperf / wget（winget 安装）与 ninja（chocolatey）路径
#   3. Zephyr SDK 1.0.1 与工具链变量
#   4. ZEPHYR_BASE
#
# 仅修改当前 PowerShell 会话的环境，不写系统环境变量。

$ErrorActionPreference = "Stop"

$platRoot = Split-Path -Parent $PSScriptRoot

# ---- 1. Python 虚拟环境（west / cmake 装在其中） -------------------------
$venvScripts = Join-Path $platRoot ".venv\Scripts"
if (-not (Test-Path (Join-Path $venvScripts "west.exe"))) {
    Write-Warning "未找到 $venvScripts\west.exe，请先执行 scripts\bootstrap.ps1 初始化。"
} else {
    $env:Path = "$venvScripts;$env:Path"
}

# ---- 2. 主机工具 PATH -----------------------------------------------------
# dtc：oss-winget.dtc 为 msys2 布局，可执行文件在包目录的 usr\bin 下
$dtcBin = Get-ChildItem `
    -Path (Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages") `
    -Filter "dtc.exe" -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match "oss-winget\.dtc.+usr\\bin" } |
    Select-Object -First 1 -ExpandProperty DirectoryName
if ($dtcBin) { $env:Path = "$dtcBin;$env:Path" }

# gperf / wget 的 winget 命令别名目录
$wingetLinks = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Links"
if (Test-Path $wingetLinks) { $env:Path = "$wingetLinks;$env:Path" }

# ninja（chocolatey）；若系统用别的方式安装 ninja，其路径已在 PATH 中即可
$chocoBin = "C:\ProgramData\chocolatey\bin"
if ((Test-Path $chocoBin) -and ($env:Path -notlike "*$chocoBin*")) {
    $env:Path = "$chocoBin;$env:Path"
}

# ---- 3. Zephyr SDK 1.0.1（与 zephyr/SDK_VERSION 锁定版本一致） ------------
$sdkDir = if ($env:ZEPHYR_SDK_INSTALL_DIR) {
    $env:ZEPHYR_SDK_INSTALL_DIR
} else {
    "E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1"
}
if (-not (Test-Path (Join-Path $sdkDir "sdk_version"))) {
    Write-Warning "ZEPHYR_SDK_INSTALL_DIR 无效: $sdkDir"
} else {
    $env:ZEPHYR_SDK_INSTALL_DIR = $sdkDir
    $env:ZEPHYR_TOOLCHAIN_VARIANT = "zephyr"
}

# ---- 4. ZEPHYR_BASE -------------------------------------------------------
$env:ZEPHYR_BASE = Join-Path $platRoot "zephyr"

# ---- 状态输出 -------------------------------------------------------------
Write-Host "[env] workspace : $platRoot"
Write-Host "[env] venv west  : $(& west --version 2>$null)"
Write-Host "[env] cmake      : $((cmake --version | Select-Object -First 1) 2>$null)"
Write-Host "[env] ninja      : $((ninja --version) 2>$null)"
Write-Host "[env] dtc        : $(((dtc --version) 2>$null | Out-String).Trim())"
Write-Host "[env] SDK        : $env:ZEPHYR_SDK_INSTALL_DIR"
Write-Host "[env] ZEPHYR_BASE: $env:ZEPHYR_BASE"
