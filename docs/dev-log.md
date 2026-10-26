# 开发记录（倒序，最新在上）

> 规则：每次改动完成后**在顶部追加**一条，字段固定（目标 / 改动 / 关键决策 / 验证 / 遗留）。
> SOP 见 [development-sop.md](./development-sop.md)。
> 本文件是项目唯一的变更事实来源：AI 与人均以此为准，不依赖口头记忆。

---

## 2026-10-21 · 应用开发层中间件交付（Modbus / JSON+CBOR / KV / UWB SR040）

**目标**：在应用开发层补齐四类中间件，并完成三块实板的构建验证。
用户确认的口径：
- Modbus 仅做主站（client），RTU over UART，避开 console 串口；
- 数据中间件 JSON 与 CBOR 均要；
- 存储用 Zephyr settings（与既有 `plat_kv` 对齐），不自研 KV；
- 图形中间件 LVGL 暂不集成，改为接入 NXP UWB SR040（SDK `UWBIOT_SR040_Zephyr_v05.09.05`）；
- SR040 "先不绑板"：先做模块集成 + `plat_uwb` 封装 + 编译验证，overlay 引脚待接线后补。

**改动**：

- 新增中间件
  - `platform/include/platform/plat_modbus.h`、`platform/middleware/communication/plat_modbus.c`、`Kconfig`
  - `platform/include/platform/plat_data.h`、`platform/middleware/data/plat_data.c`、`Kconfig`
  - `platform/include/platform/plat_uwb.h`、`platform/middleware/uwb/plat_uwb.c`、`Kconfig`
  - `platform/middleware/Kconfig`：改为 `rsource communication/data/uwb/storage`；保留 `PLAT_MW_GUI` 骨架并注明暂不集成
- 存储中间件重写
  - `platform/middleware/storage/plat_kv.c`：通用注册表（16 项 / 64B 值 / 24B 键）+ `SETTINGS_STATIC_HANDLER_DEFINE(plat, "plat", ...)`
  - `platform/include/platform/plat_kv.h`：新增 `PLAT_KV_MAX_VAL_LEN`、blob/delete/commit、boot_count
- SR040 模块集成
  - `modules/uwb/uwbiot/`：自原 SDK `uwbiot-top` 整体拷入（约 3.8MB）
  - `modules/uwb/uwbiot/boards/Host/zephyr_generic/`：新增通用板级层（由 nrf52840dk 适配）
  - `modules/uwb/uwbiot/zephyr/Kconfig.uwb`：新增 `NXP_UWB_HOST_CUSTOM`（DT 节点存在时默认 y）
  - `modules/uwb/uwbiot/dts/bindings/nxp,uwb-sr040-device.yaml`：拷贝绑定
- 应用接线
  - `app/src/app_middleware_demo.{c,h}`：KV / JSON / CBOR / Modbus / UWB 五段 demo
  - `app/src/main.c`：启动时调用 `app_middleware_demo_start()`
  - `app/CMakeLists.txt`：显式加入 `src/app_middleware_demo.c`
  - `app/prj.conf`：`CONFIG_PLAT_MW_DATA=y`
  - `app/boards/esp32c6_devkitc_esp32c6_hpcore.{conf,overlay}`：UART1 GPIO4/5 + `modbus0` 子节点、`CONFIG_PLAT_MW_COMMUNICATION=y`
  - `app/boards/stm32f407ve_custom.{conf,overlay}`：USART2 PA2/PA3 + `modbus0` 子节点、Modbus 开关
  - `app/build_all.ps1`、`zephyr_platform.code-workspace`：`ZEPHYR_EXTRA_MODULES` 加入 `modules/uwb/uwbiot`
- 构建系统
  - `platform/zephyr/CMakeLists.txt`：按 Kconfig 引入三个中间件源；UWB 增加 SDK include 路径

**关键决策 / 踩坑**：

1. **zcbor 不能显式链接**：`zephyr_library_link_libraries(zcbor)` 在 platform 模块阶段执行时 zcbor 目标尚不存在，
   CMake 退化为 `-lzcbor` → `cannot find -lzcbor`。最终链接的 linker group 已能解析符号，删除该行即可。
2. **v4.4 settings 符号改名**：`SETTINGS_FS` 已不存在，板级 conf 改为 `CONFIG_SETTINGS_FILE=y` + `CONFIG_SETTINGS_NONE=n`，
   否则 Kconfig 警告直接中止构建。
3. **日志级别宏不存在**：项目无 `CONFIG_PLATFORM_LOG_LEVEL`，统一 `LOG_MODULE_REGISTER(plat_xxx);`。
4. **Modbus API 与直觉不同**：
   - `modbus_iface_param.rx_timeout` 是 `uint32_t` 毫秒（不是 `k_timeout_t`，强转会报 "aggregate value used where an integer was expected"）；
   - `modbus_write_coils` 要 **MSB-first 位图**字节而非 `bool[]`，封装层做转换（上限 1024 线圈）。
5. **zcbor 0.9 签名**：`zcbor_new_state(..., elem_count, NULL, 0)` 共 7 参；
   `zcbor_bstr_encode(state, const struct zcbor_string *)`。
6. **SR040 SDK 适配三个硬约束**：
   - halimpl 用 `TARGET_LINK_LIBRARIES(... ${UWBIOT_BOARD_NAME})`，通用板级层必须 `PROJECT(${UWBIOT_BOARD_NAME})` 保持目标名；
   - 纯 Zephyr 构建没有 `CMAKE_SOURCE_TOP_DIR`，全部换 `ZEPHYR_NXP_UWBIOT_MODULE_DIR`；
   - `UWB_DeviceConfig.h` 要求 `DEBUG`/`NDEBUG` 二选一，通用层 CMake 定义 `NDEBUG`；
     且 SDK 原板级层依赖 `led0..led3` 别名，通用层改为空操作 LED 宏。
7. **"先不绑板"的实现方式**：`plat_uwb.c` 用 `#if DT_NODE_EXISTS(DT_NODELABEL(nxp_uwb_device))` 编译期门控，
   无节点时走桩实现返回 `-ENODEV`；uwbiot 的 `CONFIG_NXP_UWB_ZEPHYR_MODULE` 不启用，库不参与编译。

**验证**：三平台 `west build -p always` 全部成功（`app/build_all.log`）。

| 板卡 | FLASH | 占比 | RAM | 占比 |
|---|---|---|---|---|
| esp32c6_devkitc | 420,804 B | 5.02% | 189,088 B (sram0) | 37.12% |
| stm32f407ve_custom | 116,588 B | 22.24% | 30,560 B | 23.32% |
| atm3405_custom | 88,652 B | 17.11% | 28,080 B | 10.71% |

串口预期：KV 计数/回读、JSON 字符串、CBOR 字节数；Modbus 无从站时超时；UWB 未接线时打印 `-ENODEV` 提示。

**遗留**：

- SR040 未绑定任何板卡：需接线后在目标板 overlay 增加 `nxp_uwb_device` 节点（SPI + cs-gpios + irq/rdy/reset），
  并在板级 conf 打开 `CONFIG_PLAT_MW_UWB=y`。
- atm3405 未启用任何中间件（无网络、LittleFS 未适配、无 Modbus 串口），仅保持编译通过。
- LVGL 图形中间件仍为占位 Kconfig（`PLAT_MW_GUI`）。

---

## 2026-10-20 · Zephyr 4.4 迁移收尾：三平台全量构建通过

**目标**：删除构建输出后全量重建，验证 3 块实板在 Zephyr 4.4.99 下的可构建性。

**改动**：
- `app/build_all.ps1`：按板卡区分 `ZEPHYR_EXTRA_MODULES`
  （esp32c6 / stm32f407 仅 `platform`；atm3405 额外加 `modules/hal/atmosic`）
- `platform/zephyr/CMakeLists.txt`：平台库目标改为 `platform_framework`，避免与 tf-psa-crypto 的 `platform` 冲突
- 驱动 API 适配：`plat_uart`（`k_timeout_t`）、`plat_spi`（`spi_config.cs` 值类型需 `memset`）、
  `plat_i2c`（`I2C_MODE_CONTROLLER`）、`plat_net`（`net_addr_pton`、MQTT v5 `reason_code`）
- `app/boards/esp32c6_devkitc_esp32c6_hpcore.conf`：移除 `CONFIG_ZEPHYR_HAL_ESPRESSIF_MODULE_BLOBS=y`，
  改 `CONFIG_BUILD_ONLY_NO_BLOBS=y`，补 `CONFIG_FLASH=y` / `CONFIG_FLASH_MAP=y` 满足 LittleFS 静态断言

**关键决策**：atm3405 若加载 atmosic 模块以外的板卡配置，会尝试访问不存在的目录，因此必须按板卡拆分模块列表。

**验证**：三平台 `BUILD SUCCESS`。当次产物：esp32c6 9,107,084 B、stm32f407 2,843,576 B、atm3405 2,107,648 B（含 sysbuild 镜像）。

**遗留**：atm3405 的 `sys_clock_correct` 符号缺失，PM 暂时关闭规避。

---

## 2026-10-12 · 板卡支持与 atm3405 烧录链路打通（据项目记忆整理）

**目标**：完成自定义板卡 bring-up，并把 Atmosic 支持从原始 `openair/` 迁出到平台模块。

**要点**：
- 板卡：`stm32u5g9b_custom`（WLCSP208 / 160MHz）、`stm32f407ve_custom`（HSI 168MHz，USART3 控制台，PB9 LED）、
  `atm3405_custom`（Cortex-M33F，uart1 控制台，gpio0 pin5 按键 / pin6-7 LED）
- `platform/zephyr/module.yml`：声明 `dts_root: .`、`board_root: .`（v4.4 不再隐式发现）
- 仓库边界：`openair/`、`zephyr/` 保持 pristine，所有项目改动落在 `platform/` 与 `app/`
- atm3405 烧录：标准 jlink runner 的 FLM `loadfile` 走 Verify 而非 Erase，无法擦除 RRAM；
  新增自定义 west runner `atmfastload`（`platform/zephyr/runners/atmfastload.py`），
  把 `fast_load.bin` 载入 SRAM 0x20000000，经共享内存命令块驱动擦除/编程，RRAM 目标地址 0x10000
- `atmfastload` 实测：EVK 上 `west flash --runner atmfastload` 成功，80,676 B

**遗留**：`atmisp` runner 依赖 openair 的 OpenOCD 脚本，本项目未提供，暂不使用。

---

## 2026-10-12 · 工程基线（据项目记忆整理）

**目标**：建立可复现的构建环境与工作区结构。

**要点**：
- Python 3.12 `.venv`（west 1.5.0 / cmake 4.4.4）；Zephyr SDK 1.0.1（arm + riscv64 GNU 14.3.0）
- `scripts/bootstrap.ps1`（最小集 / `-Full` 全量）、`env.ps1`（会话级环境）、`build.ps1` / `run.ps1`（QEMU 双架构）
- `zephyr_platform.code-workspace`：多根工作区 + 集成终端注入 `ZEPHYR_BASE` / SDK / `ZEPHYR_EXTRA_MODULES`
- 平台模块 `name: platform`，应用通过 `ZEPHYR_EXTRA_MODULES` 挂接；`app/prj.conf` 放通用配置，板级差异放 `app/boards/`

**遗留**：GUI 中间件（LVGL）未集成。
