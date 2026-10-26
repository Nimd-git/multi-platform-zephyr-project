# Zephyr 平台工程 · 二次开发 SOP

> 面向新加入的工程师与 AI 助手。
> 目标：让人（或 AI）在不破坏现有结构的前提下，快速定位代码、正确构建烧录、按统一套路新增功能。
> 配套文档：[开发记录](./dev-log.md)（每次改动必须追加）。

---

## 0. 阅读顺序（5 分钟上手）

1. 本文第 1 章「项目全景」→ 知道仓库分三层，哪些目录可以改。
2. 第 3 章「环境准备」→ 确认本机工具链、SDK、环境变量。
3. 第 4 章「构建与烧录」→ 能编过三块板、能烧录。
4. 第 6 章「二次开发标准流程」→ 按"改动类型"查对应 checklist。
5. 第 8 章「FAQ」→ 遇到报错先查这里。

**绝对红线**：`zephyr/` 是上游仓库，`modules/hal/atmosic`、`modules/uwb/uwbiot` 是厂商 SDK，
一律保持原样，不改动。项目级改动只允许落在 `platform/`、`app/`、`docs/`、根目录工作区文件。

---

## 1. 项目全景

```
<工作区根>\             # 本仓库根目录，可整体移动/改名（内部一律相对路径）
├── zephyr/                  # 上游 Zephyr 4.4.99（只读，禁止修改）
├── modules/                 # west 管理的上游模块 + 厂商 SDK（只读）
│   ├── hal/atmosic/         # Atmosic SDK（module 名 openair），仅 atm3405 使用
│   ├── uwb/uwbiot/          # NXP SR040 UWB SDK（module 名 nxp-uwbiot）
│   └── lib/zcbor, hal/stm32, hal/espressif ...  # 上游模块
├── platform/                # ★ 自研平台模块（树外 Zephyr module，name: platform）
│   ├── boards/              # 树外自定义板卡（vendor/board）
│   ├── drivers/             # 硬件抽象层：plat_gpio/uart/spi/i2c + 自研 monitor 驱动
│   ├── subsys/              # 系统服务层：设备管理/PM/FS/NET/platform shell
│   ├── middleware/          # ★ 应用开发层中间件：communication/data/storage/uwb
│   ├── include/platform/    # 平台公共头（对 app 可见）
│   ├── dts/bindings/        # 自研绑定（plat,platform-monitor）
│   ├── compat/              # 版本兼容 Kconfig
│   └── zephyr/              # module.yml / Kconfig / CMakeLists / runners
├── app/                     # ★ 应用（冒烟 + 各层 demo）
│   ├── src/                 # 应用代码（显式列入 CMakeLists）
│   ├── boards/              # 每块板的 overlay（设备树扩展）与 conf（Kconfig 差异）
│   ├── prj.conf             # 全平台通用配置
│   ├── build_all.ps1        # 三块实板批量构建脚本
│   └── build/<board>/       # 构建产物
├── scripts/                 # bootstrap/env/build/run（QEMU 双架构）
├── docs/                    # 本文档 + 开发记录
└── zephyr_platform.code-workspace  # 多根工作区（集成终端环境注入）
```

分层与代码归属：

| 层 | 目录 | 职责 |
|---|---|---|
| 硬件抽象层 | `platform/drivers/`、`platform/include/platform/plat_{gpio,uart,spi,i2c}.h` | 统一驱动 API，屏蔽厂商差异 |
| 内核服务层 | 直接用 Zephyr 原生 | 线程/队列/事件/定时器，不再封装 |
| 系统服务层 | `platform/subsys/` | 设备管理、健康监测、PM、FS、NET、shell |
| 应用开发层 | `platform/middleware/`、`app/src/` | Modbus / JSON+CBOR / settings KV / UWB(SR040)；应用生命周期与状态机 |
| 版本兼容 | `platform/compat/`、`platform/include/platform/plat_version.h` | Kconfig 旧选项迁移、API 废弃兼容 |
| 板级支持 | `platform/boards/`、`app/boards/` | 板卡定义（DT/defconfig/runner）+ 应用侧 overlay/conf |

---

## 2. 当前支持的构建目标

| 目标 | 板卡标识 | 架构 | 说明 |
|---|---|---|---|
| ESP32-C6 DevKitC-1 | `esp32c6_devkitc/esp32c6/hpcore` | RISC-V | WiFi + IPv4 + MQTT + LittleFS |
| STM32F407VET6 | `stm32f407ve_custom` | Cortex-M4 | LittleFS + Modbus；无网络 |
| ATM3405-WQK | `atm3405_custom` | Cortex-M33 | RRAM 启动，J-Link fast_load 烧录 |
| QEMU Cortex-M3 | `qemu_cortex_m3` | Cortex-M3 | 无外设，settings 用 RAM 后端 |
| QEMU RISC-V64 | `qemu_riscv64` | RISC-V64 | 同上 |
| STM32U5G9BJ | `stm32u5g9b_custom` | Cortex-M33 | 板卡已定义，未纳入批量构建 |

---

## 3. 环境准备

### 3.1 必需组件

| 组件 | 位置 / 版本 | 备注 |
|---|---|---|
| Python | 3.12 | 用于创建 `.venv` |
| 虚拟环境 | `.venv`（west 1.5.0、cmake 4.4.4、ninja） | 由 `scripts/bootstrap.ps1` 创建 |
| Zephyr SDK | `E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1` | 含 arm-zephyr-eabi / riscv64-zephyr-elf 14.3.0 |
| dtc / gperf | winget 安装 | `scripts/env.ps1` 自动找路径 |
| OpenOCD (ESP) | `C:\Users\<user>\.zinstaller\tools\openocds\openocd-esp32` | ESP32 系烧录用 |
| J-Link | SEGGER 官方驱动 | STM32 / ATM3405 烧录调试 |

### 3.2 首次初始化

```powershell
# 最小集（QEMU 冒烟）
powershell -ExecutionPolicy Bypass -File scripts\bootstrap.ps1
# 全量（真实板卡，会 west update 全部模块）
powershell -ExecutionPolicy Bypass -File scripts\bootstrap.ps1 -Full
```

### 3.3 环境变量（三处来源，务必保持一致）

| 变量 | 值 | 注入位置 |
|---|---|---|
| `ZEPHYR_BASE` | `<工作区根>\zephyr` | `scripts/env.ps1`、workspace 文件、`build_all.ps1` |
| `ZEPHYR_TOOLCHAIN_VARIANT` | `zephyr` | 同上 |
| `ZEPHYR_SDK_INSTALL_DIR` | `E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1` | 同上 |
| `ZEPHYR_EXTRA_MODULES` | `...\modules\uwb\uwbiot;...\platform` | **必须**在 `zephyr_platform.code-workspace` 的 `terminal.integrated.env.windows` 中（VS Code 集成终端）与 `app/build_all.ps1` 中（CLI） |
| `ESPRESSIF_TOOLCHAIN_PATH` | `C:\Users\<user>\.zinstaller\tools\openocds\openocd-esp32` | `build_all.ps1`；`app/CMakeLists.txt` 另有 CMake 变量注入 |
| `OPENOCD_DEFAULT_PATH` | `...\openocd-esp32\share\openocd\scripts` | 同上，确保用乐鑫 fork 脚本 |

> 注意：`ZEPHYR_EXTRA_MODULES` 不会从父进程可靠继承。VS Code 插件终端以 workspace 文件为准，
> 改完需 **Reload Window**；命令行以 `build_all.ps1` 设置的值或手动 `$env:` 为准。

### 3.4 模块挂接关系

| 模块 | 路径 | 注入方式 |
|---|---|---|
| `platform` | `platform/` | `ZEPHYR_EXTRA_MODULES` + `app/CMakeLists.txt` 中的 `list(APPEND ...)` |
| `openair`（Atmosic） | `modules/hal/atmosic` | 根 `west.yml` manifest（atm3405 专用） |
| `nxp-uwbiot`（SR040） | `modules/uwb/uwbiot` | `ZEPHYR_EXTRA_MODULES`（三块板均注入；无 DT 节点时仅编译 Kconfig） |

`platform/zephyr/module.yml` 关键点：`dts_root: .`、`board_root: .`（v4.4 不再隐式发现模块 DTS/板卡目录），
以及自定义 west runner `atmfastload`。

---

## 4. 构建与烧录

### 4.1 三块实板批量构建（推荐）

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File app\build_all.ps1
```

- 逐板 `pristine` 全量构建，日志：`app/build_all.log`
- 单板构建命令（等价）：

```powershell
# 以下命令假设当前目录为工作区根（仓库内部一律使用相对路径）
$env:ZEPHYR_BASE="$PWD\zephyr"
$env:ZEPHYR_SDK_INSTALL_DIR='E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1'
$env:ZEPHYR_TOOLCHAIN_VARIANT='zephyr'
$env:ZEPHYR_EXTRA_MODULES="$PWD\modules\uwb\uwbiot;$PWD\platform"
$env:PATH="$PWD\.venv\Scripts;"+$env:PATH

# esp32c6
west build -p always -b esp32c6_devkitc/esp32c6/hpcore -d app/build/esp32c6_devkitc app
# stm32f407
west build -p always -b stm32f407ve_custom -d app/build/stm32f407ve_custom app
# atm3405（ExtraModules 需追加 atmosic）
$env:ZEPHYR_EXTRA_MODULES="$PWD\modules\hal\atmosic;$PWD\modules\uwb\uwbiot;$PWD\platform"
west build -p always -b atm3405_custom -d app/build/atm3405_custom app
```

产物：`app/build/<board>/zephyr/zephyr.elf`（`.bin` / `.hex` 同目录）。

### 4.2 QEMU 双架构（脚本）

```powershell
powershell -File scripts\build.ps1 -Target all      # 产物在 build/qemu_*
powershell -File scripts\run.ps1  -Board qemu_cortex_m3   # Ctrl-A 然后 X 退出
```

### 4.3 烧录

| 板卡 | 命令 | Runner / 设备 | 注意 |
|---|---|---|---|
| esp32c6 | `west flash -d app/build/esp32c6_devkitc` | `esp32`（内部调用 esptool） | USB-JTAG 与串口共用同一 USB 口，串口监视与调试不能同时用 |
| stm32f407 | `west flash -d app/build/stm32f407ve_custom` | `jlink`，`--device=STM32F407VE` | 需 SWDIO(PA13)/SWCLK(PA14)，建议接 NRST |
| atm3405 | `west flash -d app/build/atm3405_custom --runner atmfastload` | `atmfastload`（默认），J-Link `ATM3405-WQK` | 标准 jlink runner 不可用（FLM 擦除问题）；必须让 platform 模块可被发现，否则报 "does not support runner" |

调试配置参考：`app/debug/openocd-esp32c3.cfg`、`app/debug/jlink_fastload_program.jlink`。

### 4.4 Workbench（VS Code）集成

1. 用 `zephyr_platform.code-workspace` 打开工作区。
2. 集成终端已带 `ZEPHYR_BASE` / SDK / `ZEPHYR_EXTRA_MODULES`，可直接 `west build`。
3. `app/.vscode/settings.json` 的 build 配置项（board、build-dir、runner）由**用户自行维护**，AI 不代改。

---

## 5. 板级配置约定

每块板在应用侧有两个文件，命名 `<board>.overlay` / `<board>.conf`
（ESP32 带核限定符时用完整标识，如 `esp32c6_devkitc_esp32c6_hpcore.overlay`）：

| 文件 | 作用 | 典型内容 |
|---|---|---|
| `app/boards/<board>.overlay` | 设备树扩展 | 外设节点、pinctrl、flash 分区、`fstab`、`modbus0` 子节点、`plat-led0` 别名 |
| `app/boards/<board>.conf` | 板级 Kconfig 差异 | 外设使能、FS/网络、中间件开关、settings 后端 |

板卡自身定义（不属于应用）在 `platform/boards/<vendor>/<board>/`：
`board.yml`、`<board>.dts`、`<board>_defconfig`、`Kconfig.<board>`、`board.cmake`。

**通用配置放 `app/prj.conf`，板级差异放板级 conf，不要混放。**

### 5.1 关键 DT 约定

- LED 演示统一通过别名 `plat-led0`（映射到板级 `led0` 或自定义 LED 节点）。
- Modbus 主站：在目标 UART 节点下挂子节点，节点名即 `iface_name`：

```dts
&usart2 {
	status = "okay";
	modbus0: modbus0 {
		compatible = "zephyr,modbus-serial";
		status = "okay";
	};
};
```

- UWB SR040（接线后才启用）：模板见 `platform/include/platform/plat_uwb.h` 头部注释，
  节点 label 必须为 `nxp_uwb_device`，`compatible = "nxp,uwb-sr040-device"`，
  挂在使用 SPI 总线下（总线上需 `cs-gpios`），并声明 `uwb-irq-gpios` / `uwb-rdy-gpios` / `uwb-reset-gpios`。

---

## 6. 二次开发标准流程

### 6.0 通用五步

1. 判断改动归属层（驱动/服务/中间件/应用/板级）。
2. 改代码 + Kconfig + CMake 三处同步（平台模块用 `zephyr_library_sources_ifdef`，应用需显式加入 `target_sources`）。
3. 在 `app/src/app_middleware_demo.c` 或对应 demo 中加最小验证路径（返回码 + 日志）。
4. 跑三平台构建（`build_all.ps1`）。
5. 追加 `docs/dev-log.md` 记录。

### 6.1 新增平台 API（驱动 / 服务）

```
platform/include/platform/plat_xxx.h          # 1) 公共头：结构体 + API + 注释
platform/drivers/plat_xxx.c 或 subsys/...     # 2) 实现，LOG_MODULE_REGISTER(plat_xxx);
platform/drivers/Kconfig 或 subsys/Kconfig    # 3) Kconfig，depends on PLATFORM_SERVICES
platform/zephyr/CMakeLists.txt                # 4) zephyr_library_sources_ifdef(CONFIG_PLAT_XXX ...)
app/src/app_xxx_demo.c                        # 5) 最小验证
docs/dev-log.md                               # 6) 记录
```

注意：
- 日志统一 `LOG_MODULE_REGISTER(plat_xxx);`，不要引用不存在的 `CONFIG_PLATFORM_LOG_LEVEL`。
- 文件头写 SPDX + 中文职责说明。

### 6.2 新增中间件

目录结构固定为：

```
platform/middleware/<类别>/
├── Kconfig                 # config PLAT_MW_<NAME>，select 上游依赖
└── plat_<name>.c
platform/include/platform/plat_<name>.h
```

步骤：
1. `platform/middleware/Kconfig` 里 `rsource "<类别>/Kconfig"`。
2. CMake 用 `zephyr_library_sources_ifdef(CONFIG_PLAT_MW_<NAME> ...)`。
3. **不要**显式 `zephyr_library_link_libraries(<上游模块目标>)`——模块目标创建顺序在 platform 之后，
   会被当作 `-l<name>` 导致链接失败；Zephyr 最终链接已把各 zephyr_library 归档放入同一 linker group。
4. 板级 conf 按需打开开关（默认 `default n`，避免拖累未接线板卡）。
5. 在 `app_middleware_demo.c` 增加 `#if defined(CONFIG_...)` 门控的 demo。

### 6.3 新增板卡

1. `platform/boards/<vendor>/<board>/` 放置：`board.yml`、`<board>.dts`、`<board>_defconfig`、
   `Kconfig.<board>`、`board.cmake`（含 runner）。
2. 复用厂商 HAL 已有的 pinctrl dtsi（在 `modules/hal/<vendor>/dts/` 下确认器件型号文件名）。
3. 应用侧新增 `app/boards/<board>.overlay` 与 `.conf`。
4. `app/build_all.ps1` 的 `$boards` 增加一项，并确认 `ZEPHYR_EXTRA_MODULES` 组合正确。
5. `app/smoke.yaml` 的 `platform_allow` 加板卡标识。
6. 构建 → 烧录 → 串口确认 `Platform smoke started`。

### 6.4 接入外部模块 / SDK

1. 拷贝到 `modules/<类别>/<name>/`，确认其 `zephyr/module.yml` 的 `name`、`cmake`、`kconfig`、
   `settings.dts_root/board_root/soc_root`。
2. 加入 `ZEPHYR_EXTRA_MODULES`：同时更新 `app/build_all.ps1` 与 workspace 文件。
3. 生成的环境变量名为 `ZEPHYR_<NAME大写>_MODULE_DIR`（如 `nxp-uwbiot` → `ZEPHYR_NXP_UWBIOT_MODULE_DIR`）。
4. 若 SDK 板级层依赖特定开发板（nRF 等），需新增通用板级层并适配：
   - CMake 目标名必须等于 `UWBIOT_BOARD_NAME` 之类的 SDK 期望名（用 `PROJECT(${...})`）。
   - 不要使用 `CMAKE_SOURCE_TOP_DIR`（纯 Zephyr 构建未定义），改用 `ZEPHYR_<X>_MODULE_DIR`。
   - 清除对 `led0..led3` 等 DT 别名的硬依赖。
5. 平台侧新增 `plat_<name>.h/.c` 封装 + Kconfig 门控；无 DT 节点时返回 `-ENODEV` 桩实现。

### 6.5 修改 Kconfig / 配置

- 上游选项在 v4.4 有改名（例：`SETTINGS_FS` → `SETTINGS_FILE`）。改前先在本树
  `zephyr/` 下 grep 符号是否存在，避免 "attempt to assign the value 'y' to the undefined symbol"。
- 平台开关统一 `depends on PLATFORM_SERVICES`，中间件默认 `n`，服务默认 `y`。
- 在应用侧只覆盖差异项，通用项留在 `prj.conf`。

---

## 7. 平台 API 速查

| 头文件 | 能力 | 关键接口 |
|---|---|---|
| `platform/plat_version.h` | 版本与兼容宏 | `PLAT_FORMAT_VERSION`、`PLAT_DEPRECATED_SINCE()` |
| `platform/plat_arch.h` | 架构标识 | `PLAT_ARCH_NAME` |
| `platform/plat_gpio.h` | GPIO | 输出/输入/中断/`plat_led_*` |
| `platform/plat_uart.h` | UART | 轮询收发、中断接收（v4.4 超时为 `k_timeout_t`） |
| `platform/plat_spi.h` | SPI | 全双工传输（`spi_config.cs` 为值类型，需 `memset` 清零） |
| `platform/plat_i2c.h` | I2C | 读写/寄存器（主模式常量 `I2C_MODE_CONTROLLER`） |
| `platform/plat_device.h` | 设备管理 | 设备枚举、健康查询 |
| `platform/plat_health.h` | 健康监测 | 错误计数、状态上报 |
| `platform/plat_pm.h` | 电源管理 | `pm_device_runtime_*` 封装 |
| `platform/plat_fs.h` | 文件系统 | LittleFS 挂载/读写 |
| `platform/plat_net.h` | 网络 | IPv4 + MQTT 单链路（esp32c6） |
| `platform/plat_monitor.h` | 自研骨架驱动 | `plat_monitor_ping()` |
| `platform/plat_kv.h` | 存储 KV | `plat_kv_get/set_u32`、`plat_kv_get/set_blob`、`plat_kv_delete/commit`、`plat_kv_boot_count` |
| `platform/plat_data.h` | 数据序列化 | JSON `plat_data_json_encode/decode`；CBOR `plat_data_cbor_*` 流式编码 |
| `platform/plat_modbus.h` | Modbus RTU 主站 | `plat_modbus_client_init/deinit`、`read_holding/read_input`、`write_register(s)/write_coil(s)` |
| `platform/plat_uwb.h` | UWB SR040 | `plat_uwb_init/deinit`、`get_device_info`、`session_start/stop/deinit`、`set_notif_cb` |

### 7.1 中间件使用要点

**存储（settings）**
- 平台 handler 子树为 `plat`，键名建议 `plat/<模块>/<键>`。
- QEMU 用 `CONFIG_SETTINGS_NONE=y`（RAM）；真实板卡 `CONFIG_SETTINGS_FILE=y` + `CONFIG_SETTINGS_NONE=n`，落 `/lfs`。
- `plat_kv_set_blob` 同时更新运行时缓存并持久化，`-ENOENT` 视为 RAM 后端成功。

**JSON / CBOR**
- JSON 使用 Zephyr `json_obj_descr` 描述符；`bool` 字段用 `JSON_TOK_TRUE`。
- CBOR 为流式 API：`encoder_init → map_open(n) → key_str/put_* → map_close → finish(&len)`。
- zcbor 0.9 签名：`zcbor_new_state(states, n, buf, len, elem_count, NULL, 0)`；
  `zcbor_bstr_encode(state, const struct zcbor_string *)`。

**Modbus**
- 先有 DT 子节点，再 `modbus_iface_get_by_name("modbus0")`。
- `modbus_iface_param.rx_timeout` 单位是**毫秒（uint32_t）**，不是 `k_timeout_t`。
- `modbus_write_coils` 需要 **MSB-first 位图**字节，平台封装已把 `bool[]` 转换好。

**UWB SR040**
- 无 `nxp_uwb_device` 节点时所有 API 返回 `-ENODEV`（桩实现，编译期门控）。
- 当前示例范围：init → 设备信息 → 会话启动/停止。

---

## 8. 验证清单

每次改动后至少完成：

- [ ] `app/build_all.ps1` 三块板全绿（日志无 `BUILD FAILED`）
- [ ] 关注占用变化：FLASH/RAM 不应异常暴涨（参考基线见 dev-log）
- [ ] 烧录后串口出现 `Platform smoke started | arch=... | zephyr=...`
- [ ] shell 命令可用：`platform ver / devices / uptime / led on|off|toggle / kv / ping`，`app state / app fault`
- [ ] 中间件 demo 日志符合预期：
  - KV：`KV counter set=0 value=N`、`KV blob readback`
  - JSON：`JSON: {"seq":42,...}`
  - CBOR：`CBOR encoded N bytes`
  - Modbus：无从站时超时报错属**预期**
  - UWB：未接线时 `SR040 not wired ... -19` 属**预期**
- [ ] 已在 `docs/dev-log.md` 追加记录

---

## 9. FAQ / 常见错误

| 现象 | 原因 | 处理 |
|---|---|---|
| 链接 `cannot find -lzcbor`（或 `-l<模块>`） | platform 模块先于上游模块创建，显式链接变成纯库名 | 删除 `zephyr_library_link_libraries(<模块目标>)`，依赖最终链接的 linker group |
| Kconfig `attempt to assign ... undefined symbol SETTINGS_FS` | v4.4 改名 | 改用 `CONFIG_SETTINGS_FILE=y`，并 `CONFIG_SETTINGS_NONE=n` |
| `undefined reference to app_xxx` | `app` 目标源文件是显式列表 | 在 `app/CMakeLists.txt` 的 `target_sources(app PRIVATE ...)` 中加入 |
| 板卡 `does not support runner atmfastload` | 模块未被 west 发现（runners.yaml 不够，需模块可见） | 确认 `ZEPHYR_EXTRA_MODULES` 含 `platform`，或由 manifest 管理 |
| `modbus_iface_get_by_name` 返回 `-ENODEV` | overlay 缺 `zephyr,modbus-serial` 子节点，或节点名与 `iface_name` 不一致 | 在目标 UART 下加 `modbus0` 子节点 |
| 编译报 `CONFIG_PLATFORM_LOG_LEVEL` 未定义 | 该项目不存在该宏 | 统一 `LOG_MODULE_REGISTER(plat_xxx);` |
| `zcbor_new_state` 参数数量不符 | 本树 zcbor 0.9，7 参数 | 末尾补 `NULL, 0` |
| `zcbor_bstr_encode` 类型不匹配 | 0.9 接受 `struct zcbor_string *` | 构造 `{ .value, .len }` 传入 |
| 集成终端找不到 `west` / 缺 SDK 变量 | VS Code 主进程未继承新环境变量 | 以 workspace 文件 `terminal.integrated.env.windows` 为准，Reload Window |
| `env.ps1` 后 west 报错无输出却 exit 1 | 脚本顶部 `$ErrorActionPreference='SilentlyContinue'` 吞掉 stderr | 诊断时在其后改 `Continue`，或用 `powershell -File` 运行脚本 |
| atm3405 标准 jlink 烧录失败 | FLM `loadfile` 走 Verify 而非 Erase | 用 `atmfastload` runner（SRAM fast_load 协议，RRAM 地址 0x10000） |
| uwbiot 编译报 `UWB_DeviceConfig.h` `#error` | 需 `DEBUG` 或 `NDEBUG` 二选一 | 通用板级层 CMake 已定义 `NDEBUG` |
| 改完代码构建仍用旧产物 | Workbench 的 `--pristine never` | 用 `-p always` 或删除 `app/build/<board>` |

---

## 10. 编码与仓库约定

- 脚本（`bootstrap/env/build/run/build_all.ps1`）保存为 **UTF-8 with BOM**，否则 Windows PowerShell 5.1 可能乱码。
- C 文件头：`SPDX-License-Identifier: Apache-2.0` + 中文职责注释。
- 日志：`LOG_MODULE_REGISTER(<module>);` 后直接用 `LOG_INF/WRN/ERR`。
- 平台公共头放 `platform/include/platform/`，应用私有头放 `app/src/`。
- 不引入不必要抽象：新增封装前先确认上游已有能力。
- **禁止改动**：`zephyr/`、`modules/`（厂商 SDK）、`C:\Users\<user>\.zinstaller\env.ps1`。
- `app/.vscode/settings.json`、根 `.vscode/settings.json` 由用户维护，AI 只给建议不落盘。

---

## 11. AI 协作特别约定

AI 接手本工程时，按以下顺序执行：

1. 读本文 + `docs/dev-log.md` 最近三条记录。
2. 确认改动目标属于 `platform/` 还是 `app/`，列出"代码 + Kconfig + CMake + demo + 文档"五件套。
3. 改前先 `Read` 目标文件与相关头文件，不凭记忆写 API。
4. 任何涉及环境变量/模块挂接的改动，必须同时检查：
   `app/build_all.ps1`、`zephyr_platform.code-workspace`、`platform/zephyr/CMakeLists.txt`。
5. 构建命令统一用 `build_all.ps1`（Windows 下 Shell 偶发 `SpawnFailed`，可用 `dangerouslyDisableSandbox` 或改脚本执行）。
6. 构建后检查日志关键字：`BUILD FAILED` / `error:` / `undefined reference` / `warning: attempt to assign`。
7. **必须**在 `docs/dev-log.md` 顶部追加本次记录，包含：日期、目标、改动文件、关键决策、验证结果、遗留问题。
8. 不臆造硬件参数；引脚、器件型号、runner 设备名以现有 DT/board.cmake 为准。

---

## 12. 开发记录维护规则

`docs/dev-log.md` 采用**倒序**（最新在上），每条固定字段：

```markdown
## YYYY-MM-DD · <一句话主题>

**目标**：...
**改动**：
- `path/to/file`：...

**关键决策/踩坑**：...
**验证**：构建结果 + 烧录/串口结果
**遗留**：...（无则写"无"）
```

提交前自检：记录中的文件路径真实存在、构建数字与日志一致、遗留项明确责任人/触发条件。

---

_最后更新：2026-10-21_
