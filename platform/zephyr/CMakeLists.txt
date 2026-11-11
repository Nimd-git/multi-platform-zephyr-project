# SPDX-License-Identifier: Apache-2.0
#
# Platform Framework —— 树外模块构建入口
# 对应架构图：硬件抽象层（统一设备模型/驱动API标准化）+ 系统服务层 +
#             中间件（应用开发层）+ 版本兼容与适配。

set(PLATFORM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT CONFIG_PLATFORM_SERVICES)
  return()
endif()

zephyr_library_named(platform_framework)

# 平台公共头对应用及模块自身可见（plat_version/plat_gpio/...）
zephyr_include_directories(${PLATFORM_DIR}/include)

# ---- 系统服务层：核心服务 + 设备管理/健康监测 + PM/FS/NET ------------------
zephyr_library_sources(
  ${PLATFORM_DIR}/subsys/platform_core.c
  ${PLATFORM_DIR}/subsys/device_mgmt.c
)
zephyr_library_sources_ifdef(CONFIG_PLAT_PM
  ${PLATFORM_DIR}/subsys/plat_pm.c
)
zephyr_library_sources_ifdef(CONFIG_PLAT_FS
  ${PLATFORM_DIR}/subsys/plat_fs.c
)
if(CONFIG_FILE_SYSTEM_LITTLEFS)
  zephyr_library_link_libraries(LITTLEFS)
endif()
zephyr_library_sources_ifdef(CONFIG_PLAT_NET
  ${PLATFORM_DIR}/subsys/plat_net.c
)

# ---- 硬件抽象层：驱动 API 标准化（GPIO/UART/SPI/I2C 统一封装） -------------
zephyr_library_sources_ifdef(CONFIG_PLAT_GPIO
  ${PLATFORM_DIR}/drivers/plat_gpio.c
)
zephyr_library_sources_ifdef(CONFIG_PLAT_UART
  ${PLATFORM_DIR}/drivers/plat_uart.c
)
zephyr_library_sources_ifdef(CONFIG_PLAT_SPI
  ${PLATFORM_DIR}/drivers/plat_spi.c
)
zephyr_library_sources_ifdef(CONFIG_PLAT_I2C
  ${PLATFORM_DIR}/drivers/plat_i2c.c
)

# ---- 硬件抽象层：自研驱动骨架（设备树实例化，演示统一设备模型） ----------
zephyr_library_sources_ifdef(CONFIG_PLAT_MONITOR
  ${PLATFORM_DIR}/drivers/skeleton/monitor.c
)

# ---- 中间件：存储 KV（基于 settings） --------------------------------------
zephyr_library_sources_ifdef(CONFIG_PLAT_MW_STORAGE
  ${PLATFORM_DIR}/middleware/storage/plat_kv.c
)

# ---- 中间件：通信（Modbus RTU 主站） ---------------------------------------
zephyr_library_sources_ifdef(CONFIG_PLAT_MW_COMMUNICATION
  ${PLATFORM_DIR}/middleware/communication/plat_modbus.c
)

# ---- 中间件：数据（JSON + CBOR） -------------------------------------------
if(CONFIG_PLAT_MW_DATA)
  zephyr_library_sources(
    ${PLATFORM_DIR}/middleware/data/plat_data.c
  )
  # zcbor 头文件/归档由 zcbor 模块提供（CONFIG_ZCBOR）；
  # 无需显式 link：最终链接阶段所有 zephyr_library 归档处于同一 linker group，
  # 且 zcbor 目标在 platform 模块之后才创建，显式链接会被当成 -lzcbor 导致链接失败
endif()

# ---- 中间件：UWB（NXP SR040） ----------------------------------------------
# 注意：厂商 SDK（modules/uwb/uwbiot）不做任何修改。SDK 模块构建路径（其
# zephyr/CMakeLists.txt）定义了 UWBIOT_USE_FTR_FILE 却不提供 uwb_iot_ftr.h 的
# include 路径（上游仅 Nordic 独立工程流程提供），此处把 SDK 自带的静态特性头
# 目录（project/Nordic_Zephyr，SR040 演示配置）补进全局 include，供 halimpl
# 等库源码编译使用。
if(CONFIG_NXP_UWB_ZEPHYR_MODULE)
  zephyr_include_directories(
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/project/Nordic_Zephyr
  )
endif()

if(CONFIG_PLAT_MW_UWB)
  # plat_uwb.c 经 SDK UwbApi.h → UwbApi_Types.h → uci_defs.h 等多层 include 链，
  # 需把 SDK 所有头文件目录加进来（SDK 自身的 libs/CMakeLists.txt 只对各自目标
  # target_include_directories，不传给外部库）。
  zephyr_include_directories(
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/uwb-iot/uwb_api/fira_generic
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/uwb-iot/uwb_api/proprietary/SR040
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/uwb-iot/uwb_core/include
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/halimpl/inc
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/halimpl/tml
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/halimpl/transport/SPI
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/uci-core/inc
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/libs/uci-core/inc/SR040
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/boards
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/boards/Host/zephyr_generic
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/boards/Host/zephyr_generic/include
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/boards/uwbs/SR040
    ${ZEPHYR_NXP_UWBIOT_MODULE_DIR}/boards/uwbs_driver
  )
  zephyr_library_sources(
    ${PLATFORM_DIR}/middleware/uwb/plat_uwb.c
  )
  # platform_framework 调用 SDK 的 UwbApi_* 系列符号，需显式链接 SDK 静态库。
  # 这些 SDK 库由 nxp-uwbiot 模块以 ADD_LIBRARY 创建（uwb_api 等在配置期即存在），
  # 但 halimpl 经 zephyr_library_named() 延迟创建，配置期 if(TARGET halimpl) 为 false，
  # 故用 if(TARGET) 守卫：存在者直接链接；缺失者（halimpl）经 SDK 库内部的
  # TARGET_LINK_LIBRARIES 传递依赖（uwb_api -> uwb_core/uci_core -> halimpl/mwlog/...）
  # 在链接期被拉入。注意：zephyr_library_named 内部已对 platform_framework 用过
  # target_link_libraries 的 PUBLIC 关键字签名，故此处也必须用关键字（PUBLIC），
  # 不能用 plain 签名（CMake 要求同一目标全部 keyword 或全部 plain）。
  foreach(_lib uwb_api uwb_core uci_core mwlog NRF52840_SR040 halimpl)
    if(TARGET ${_lib})
      target_link_libraries(platform_framework PUBLIC ${_lib})
    endif()
  endforeach()
endif()
