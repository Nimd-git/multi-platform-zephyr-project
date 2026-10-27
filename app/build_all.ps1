#requires -version 5.1
$ErrorActionPreference = 'Continue'

# 以脚本所在位置推导工作区根，与文件夹名解耦（重命名后无需修改）
$ws = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

# 统一环境入口：ZEPHYR_SDK_INSTALL_DIR / ZEPHYR_BASE / PATH 只在 scripts\env.ps1 维护，
# 避免同一路径在本文件重复硬编码。其他开发者只需配置自己的 SDK 路径。
. (Join-Path $ws 'scripts\env.ps1')
$ErrorActionPreference = 'Continue'

$platformModule = Join-Path $ws 'platform'
$atmosicModule = Join-Path $ws 'modules\hal\atmosic'
$uwbiotModule = Join-Path $ws 'modules\uwb\uwbiot'

$appDir = Join-Path $ws 'app'
$boards = @(
    @{ Name='esp32c6_devkitc'; Board='esp32c6_devkitc/esp32c6/hpcore'; Dir='build/esp32c6_devkitc'; ExtraModules="$uwbiotModule;$platformModule" },
    @{ Name='stm32f407ve_custom'; Board='stm32f407ve_custom'; Dir='build/stm32f407ve_custom'; ExtraModules="$uwbiotModule;$platformModule" },
    @{ Name='atm3405_custom'; Board='atm3405_custom'; Dir='build/atm3405_custom'; ExtraModules="$atmosicModule;$uwbiotModule;$platformModule" }
)

$logFile = "$appDir\build_all.log"
Remove-Item -Force $logFile -ErrorAction SilentlyContinue

foreach ($b in $boards) {
    $buildDir = Join-Path $appDir $b.Dir
    $env:ZEPHYR_EXTRA_MODULES = $b.ExtraModules
    "===== Building $($b.Name) =====" | Tee-Object -Append -FilePath $logFile
    try {
        west build -p always -b $b.Board -d $buildDir $appDir 2>&1 | Tee-Object -Append -FilePath $logFile
        if ($LASTEXITCODE -ne 0) {
            "BUILD FAILED for $($b.Name) with exit code $LASTEXITCODE" | Tee-Object -Append -FilePath $logFile
        } else {
            "BUILD SUCCESS for $($b.Name)" | Tee-Object -Append -FilePath $logFile
        }
    } catch {
        "BUILD EXCEPTION for $($b.Name): $_" | Tee-Object -Append -FilePath $logFile
    }
}
