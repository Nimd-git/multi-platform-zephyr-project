# SPDX-License-Identifier: Apache-2.0
#
# 冒烟应用（应用开发层）。
# 必须在 find_package(Zephyr) 之前挂接树外平台模块。

cmake_minimum_required(VERSION 3.20.0)

list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../platform)

# Atmosic SDK（openair）提供 ATM34xx SoC 定义。当前工作区的 .west/config 以
# zephyr/west.yml 为清单，根 west.yml 中的 atmosic 项目不会被 west 发现，
# 故仅 atm3405 板卡显式挂接该模块（与 app/build_all.ps1 按板卡拆分的策略一致），
# 避免其他板卡加载 Atmosic 模块。
if(BOARD STREQUAL "atm3405_custom")
  list(APPEND ZEPHYR_EXTRA_MODULES ${CMAKE_CURRENT_SOURCE_DIR}/../modules/hal/atmosic)
endif()

# 将 esp-openocd 位置注入 CMake 变量（board.cmake 只认 CMake 变量，不认环境变量）。
# 路径来源（优先级从高到低，均不写死用户名/盘符）：
#   1) 环境变量 ESPRESSIF_TOOLCHAIN_PATH（指向 openocds 根，其下含 openocd-esp32）
#   2) %USERPROFILE%\.zinstaller\tools\openocds（Workbench 默认安装位置）
# 必须同时满足 board.cmake 的两步：
#   1) 正则校验 OPENOCD 路径以 ${ESPRESSIF_TOOLCHAIN_PATH}/ 开头（需正斜杠路径）
#   2) find_program 在 ${ESPRESSIF_TOOLCHAIN_PATH}/openocd-esp32/bin 下找到 openocd.exe
if(DEFINED ENV{ESPRESSIF_TOOLCHAIN_PATH} AND NOT "$ENV{ESPRESSIF_TOOLCHAIN_PATH}" STREQUAL "")
  file(TO_CMAKE_PATH "$ENV{ESPRESSIF_TOOLCHAIN_PATH}" _esp_openocd_root)
else()
  file(TO_CMAKE_PATH "$ENV{USERPROFILE}/.zinstaller/tools/openocds" _esp_openocd_root)
endif()
# 兼容旧写法：变量指向 openocd-esp32 子目录时上溯到 openocds 根
if(_esp_openocd_root MATCHES "openocd-esp32/?$")
  get_filename_component(_esp_openocd_root "${_esp_openocd_root}" DIRECTORY)
endif()
set(ESPRESSIF_TOOLCHAIN_PATH "${_esp_openocd_root}"
    CACHE PATH "Espressif toolchain root" FORCE)
set(OPENOCD "${_esp_openocd_root}/openocd-esp32/bin/openocd.exe"
    CACHE FILEPATH "OpenOCD executable" FORCE)
# 脚本搜索路径必须指向乐鑫 fork 版脚本（含 create_esp_target 等私有命令），
# 否则 Zephyr 会自动混入 SDK hosttools 里的原版 openocd 脚本导致版本错配。
set(OPENOCD_DEFAULT_PATH "${_esp_openocd_root}/openocd-esp32/share/openocd/scripts"
    CACHE PATH "OpenOCD scripts search path" FORCE)

# 首选环境变量 ZEPHYR_BASE；未设置时回退到工作区内的 zephyr/，
# 避免 pristine 构建（缓存被清空）时找不到 ZephyrConfig.cmake。
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE} ${CMAKE_CURRENT_SOURCE_DIR}/../zephyr)
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
