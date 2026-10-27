# SPDX-License-Identifier: Apache-2.0
#
# 平台工作区引导脚本（在 PowerShell 中执行）
#
# 用法:
#   powershell -ExecutionPolicy Bypass -File scripts\bootstrap.ps1            # 最小集（QEMU 双架构冒烟）
#   powershell -ExecutionPolicy Bypass -File scripts\bootstrap.ps1 -Full      # 全量模块（真实板卡/HAL）
#
# 前置条件（已装会自动跳过）:
#   - Python 3.12（py 启动器）、Git
#   - winget（用于安装 dtc / gperf；cmake 与 west 装入工作区 .venv，免管理员权限）
#   - Zephyr SDK 1.0.1 已解压，路径由 ZEPHYR_SDK_INSTALL_DIR 指定
#     （未设置时默认 E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1，仅为本机约定）
#     （版本由 zephyr\SDK_VERSION 锁定；需含 arm-zephyr-eabi 与 riscv64-zephyr-elf 工具链）

[CmdletBinding()]
param(
    [switch]$Full
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# 1. winget 安装 dtc / gperf（已安装时为快速无操作）
foreach ($pkg in @("oss-winget.dtc", "oss-winget.gperf")) {
    Write-Host "[bootstrap] winget 确保 $pkg"
    winget install --id $pkg -e --source winget `
        --accept-source-agreements --accept-package-agreements --disable-interactivity
}

# 2. 创建 Python 3.12 虚拟环境并安装 west / cmake
$venvDir = Join-Path $root ".venv"
if (-not (Test-Path (Join-Path $venvDir "Scripts\west.exe"))) {
    Write-Host "[bootstrap] 创建虚拟环境 .venv (Python 3.12)"
    py -3.12 -m venv $venvDir
    & "$venvDir\Scripts\python.exe" -m pip install --upgrade pip west cmake
}

# 3. west init：复用本地 zephyr/ 树，不重复克隆
if (-not (Test-Path ".west")) {
    Write-Host "[bootstrap] west init -l zephyr"
    west init -l zephyr
} else {
    Write-Host "[bootstrap] .west 已存在，跳过 west init"
}

# 4. 拉取模块
#    v4.4 的 Cortex-M 依赖 CMSIS_6（不是旧版 cmsis）；RISC-V 无需额外模块。
if ($Full) {
    Write-Host "[bootstrap] west update（全量，耗时较长）"
    west update
} else {
    Write-Host "[bootstrap] west update cmsis_6（QEMU 冒烟最小集）"
    west update cmsis_6
}

# 5. 注册 Zephyr SDK 的 CMake 包（setup.cmd 需要 cmake 与 7z 在 PATH）
$sevenZip = Join-Path $env:ProgramFiles "7-Zip"
if (Test-Path $sevenZip) { $env:Path = "$sevenZip;$env:Path" }
$env:Path = "$venvDir\Scripts;$env:Path"
$sdkDir = if ($env:ZEPHYR_SDK_INSTALL_DIR) {
    $env:ZEPHYR_SDK_INSTALL_DIR
} else {
    "E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1"
}
$setupCmd = Join-Path $sdkDir "setup.cmd"
if (Test-Path $setupCmd) {
    Write-Host "[bootstrap] 注册 Zephyr SDK CMake 包"
    & $setupCmd /c
}

# 6. 注册 Zephyr CMake 包 + 安装 Python 构建依赖
Write-Host "[bootstrap] west zephyr-export"
west zephyr-export

Write-Host "[bootstrap] 安装 Zephyr Python 依赖（requirements-base.txt）"
python -m pip install -r zephyr\scripts\requirements-base.txt

Write-Host ""
Write-Host "[bootstrap] 完成。新开 PowerShell 后先加载环境："
Write-Host "    . .\scripts\env.ps1"
Write-Host "构建：  powershell -File scripts\build.ps1 -Target all"
Write-Host "运行：  powershell -File scripts\run.ps1 -Board qemu_cortex_m3"
if (-not $Full) {
    Write-Host "补全所有 HAL 模块（真实板卡前执行）：west update"
}
