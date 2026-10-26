# SPDX-License-Identifier: Apache-2.0
#
# 在 QEMU 中运行已构建的冒烟应用（退出: Ctrl-A 然后 X）
#
# 用法:
#   powershell -File scripts\run.ps1 -Board qemu_cortex_m3
#   powershell -File scripts\run.ps1 -Board qemu_riscv64

[CmdletBinding()]
param(
    [ValidateSet("qemu_cortex_m3", "qemu_riscv64")]
    [string]$Board = "qemu_cortex_m3"
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "env.ps1")
$root = Split-Path -Parent $PSScriptRoot
$buildDir = Join-Path $root ("build/" + $Board)

if (-not (Test-Path (Join-Path $buildDir "build.ninja"))) {
    Write-Error "未找到构建目录 $buildDir，请先执行 scripts\build.ps1 -Target $($Board -replace 'qemu_','')"
    exit 1
}

west build -t run -d $buildDir
