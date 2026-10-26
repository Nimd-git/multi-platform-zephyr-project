#requires -version 5.1
$ErrorActionPreference = 'Continue'

$env:ZEPHYR_BASE = 'F:\src\zephyr_project\zephyr'
$env:ZEPHYR_SDK_INSTALL_DIR = 'E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1'
$env:ZEPHYR_TOOLCHAIN_VARIANT = 'zephyr'
$env:ESPRESSIF_TOOLCHAIN_PATH = 'C:\Users\12968399\.zinstaller\tools\openocds\openocd-esp32'
$env:OPENOCD_DEFAULT_PATH = 'C:/Users/12968399/.zinstaller/tools/openocds/openocd-esp32/share/openocd/scripts'
$platformModule = 'F:\src\zephyr_project\platform'
$atmosicModule = 'F:\src\zephyr_project\modules\hal\atmosic'
$uwbiotModule = 'F:\src\zephyr_project\modules\uwb\uwbiot'

# Ensure venv west is on PATH
$venvScripts = 'F:\src\zephyr_project\.venv\Scripts'
if ($env:PATH -notlike "*$venvScripts*") {
    $env:PATH = "$venvScripts;${env:PATH}"
}

$appDir = 'F:\src\zephyr_project\app'
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
