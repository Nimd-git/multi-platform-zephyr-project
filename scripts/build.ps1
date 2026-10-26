# SPDX-License-Identifier: Apache-2.0
#
# 平台冒烟应用构建脚本（Cortex-M + RISC-V 双架构）
#
# 用法:
#   powershell -File scripts\build.ps1 -Target cortex_m3
#   powershell -File scripts\build.ps1 -Target riscv64
#   powershell -File scripts\build.ps1 -Target all
#
# 可加 -Run 构建后直接进入 QEMU 运行（Ctrl-A X 退出）。

[CmdletBinding()]
param(
    [ValidateSet("cortex_m3", "riscv64", "all")]
    [string]$Target = "all",
    [switch]$Run
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "env.ps1")
$root = Split-Path -Parent $PSScriptRoot
$app  = Join-Path $root "app"

$targets = @()
if ($Target -eq "all") {
    $targets = @("qemu_cortex_m3", "qemu_riscv64")
} else {
    $targets = @("qemu_" + $Target)
}

foreach ($board in $targets) {
    $buildDir = Join-Path $root ("build/" + $board)
    Write-Host "[build] $board -> $buildDir"
    west build -b $board $app -d $buildDir --pristine
    if ($LASTEXITCODE -ne 0) {
        Write-Error "构建失败: $board"
        exit 1
    }
    if ($Run) {
        Write-Host "[run] $board"
        west build -t run -d $buildDir
    }
}

Write-Host "[build] 全部完成。产物位于 build/<board>/zephyr/zephyr.elf"
