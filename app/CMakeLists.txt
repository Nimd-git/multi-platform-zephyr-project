# SPDX-License-Identifier: Apache-2.0
#
# 冒烟应用（应用开发层）。
# 必须在 find_package(Zephyr) 之前挂接树外平台模块。

cmake_minimum_required(VERSION 3.20.0)

# atmosic 已由 west manifest（modules/hal/atmosic）管理，无需再经 ZEPHYR_EXTRA_MODULES 挂接
list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../platform)

# 将 esp-openocd 位置注入 CMake 变量（board.cmake 只认 CMake 变量，不认环境变量）。
# 必须同时满足 board.cmake 的两步：
#   1) 正则校验 OPENOCD 路径以 ${ESPRESSIF_TOOLCHAIN_PATH}/ 开头（需正斜杠路径）
#   2) find_program 在 ${ESPRESSIF_TOOLCHAIN_PATH}/openocd-esp32/bin 下找到 openocd.exe
set(ESPRESSIF_TOOLCHAIN_PATH "C:/Users/12968399/.zinstaller/tools/openocds"
    CACHE PATH "Espressif toolchain root" FORCE)
set(OPENOCD "C:/Users/12968399/.zinstaller/tools/openocds/openocd-esp32/bin/openocd.exe"
    CACHE FILEPATH "OpenOCD executable" FORCE)
# 脚本搜索路径必须指向乐鑫 fork 版脚本（含 create_esp_target 等私有命令），
# 否则 Zephyr 会自动混入 SDK hosttools 里的原版 openocd 脚本导致版本错配。
set(OPENOCD_DEFAULT_PATH "C:/Users/12968399/.zinstaller/tools/openocds/openocd-esp32/share/openocd/scripts"
    CACHE PATH "OpenOCD scripts search path" FORCE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(platform_smoke LANGUAGES C)

target_include_directories(app PRIVATE src)

target_sources(app PRIVATE
  src/main.c
  src/app_lifecycle.c
  src/app_state_machine.c
  src/app_kernel_demo.c
  src/app_services_demo.c
  src/app_middleware_demo.c
)
