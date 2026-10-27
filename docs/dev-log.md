# 开发记录（倒序，最新在上）

> 规则：每次改动完成后**在顶部追加**一条，字段固定（目标 / 改动 / 关键决策 / 验证 / 遗留）。
> SOP 见 [development-sop.md](./development-sop.md)。
> 本文件是项目唯一的变更事实来源：AI 与人均以此为准，不依赖口头记忆。

---

## 2026-10-27 · 上位机工具增加「环境变量配置」检测与一键写入

**目标**：让其他开发者首次打开工具就能看到要配哪些环境变量、各变量用途、路径示例，
并通过「一键配置缺失项」自动写入用户级注册表（HKCU），避免手写 setx 命令。

**改动（`F:\src\zephyr_workbench`）**：
- `core/probe.py`：
  - 新增 `_read_user_env(name)` 用 `winreg` 直接读 HKCU\Environment，
    区分"进程继承的 os.environ"和"用户级持久设置"。
  - 新增 `_ps_set_user_env(name, value)` 返回 PowerShell `SetEnvironmentVariable('X','Y','User')`
    命令列表（供 fix 机制自动执行）。**选 SetEnvironmentVariable 而非 setx**：后者截断 1024 字符、
    不展开 `%USERPROFILE%`、且注册表存字面量 `%USERPROFILE%` 导致工具读取后路径错。
  - 新增 `_resolve_user_profile()`、`workspace_env()` 里也改为优先 HKCU。
  - 新增 **「环境变量配置」分组** 的三条检查项：ZEPHYR_SDK_INSTALL_DIR（必须，未持久设为 MISSING）、
    ZEPHYR_TOOLCHAIN_VARIANT（必须，值不为 zephyr 为 WARN）、ESPRESSIF_TOOLCHAIN_PATH（可选，
    未安装 OpenOCD 时 WARN + 安装指引，fix 提供默位置写入）。
  - SDK 已就位但未持久设 env 时，SDK 项升级为 WARN + hint 提示换机器后可能失效。
  - `_find_openocd` 也改为 HKCU 优先，并增加 `openocds` 根目录候选（兼容新语义）。
- `core/steps.py`：WORKFLOW_TEXT 最前面新增"零、首次配置环境变量"章节，列出三条 setx 命令、
  SDK 下载链接，并告知可在工具内一键配置。

**用户视角**：
- 环境检测页现在多三条「环境变量配置」行，颜色/状态一目了然；
- 点「一键配置缺失项」会弹出确认 → 执行 PowerShell 写 HKCU → 自动重检；
- 也可看「开发流程」文本里的 setx 命令手工配。

**验证**：Python 源码语法人工通读正确；Shell 工具间歇性启动失败未能跑 py_compile 与工具 GUI 冒烟，
建议在本机 `python -m py_compile core/probe.py core/steps.py` 后运行上位机工具点一次「开始检测」+「一键配置」。

---

## 2026-10-27 · 写入用户环境变量，并同步可移植改动到上位机工具

**目标**：把本轮环境/路径改动落到本机用户级环境变量，并同步到 `F:\src\zephyr_workbench`。

**环境变量（用户级，已写入，重开终端生效）**：
- `ZEPHYR_SDK_INSTALL_DIR = E:\zephyr-sdk\toolchains\zephyr-sdk-1.0.1`
- `ZEPHYR_TOOLCHAIN_VARIANT = zephyr`
- `ESPRESSIF_TOOLCHAIN_PATH = %USERPROFILE%\.zinstaller\tools\openocds`（openocds 根，其下含
  `openocd-esp32`，与 `app/CMakeLists.txt` 的新语义一致）
- 未写入 `ZEPHYR_BASE` / `ZEPHYR_EXTRA_MODULES`：二者与具体工作区绑定，继续由
  `scripts/env.ps1` 和 `.code-workspace` 提供。

**上位机工具（F:\src\zephyr_workbench）改动**：
- `core/settings.py`：默认工作区不再写死 `F:\src\...`，改为 `ZEPHYR_WORKSPACE` 环境变量
  + `%USERPROFILE%\src\multi-platform-zephyr-project` 探测。
- `core/probe.py`：OpenOCD 提示改为只需 `ESPRESSIF_TOOLCHAIN_PATH`（指向 openocds 根），
  不再要求 `OPENOCD_DEFAULT_PATH`。
- `app.py`：工作区示例/对话框默认目录改为用户目录推导，去掉 `F:\src`。
- `ZephyrWorkbench.spec`：`pathex`、`datas`、`icon` 改为基于 `SPECPATH` 推导。
- `build-desktop.ps1` / `run_gui.bat` 检查后无绝对路径，未改动；`build/`、`dist/` 为生成物。

**验证**：上位机源码已无 `F:\src` / `E:\zephyr-sdk` 硬编码（生成目录除外）；环境变量写入命令
执行成功。未重新打包 exe，如需分发需运行 `build-desktop.ps1`。

---

## 2026-10-27 · 绝对路径清理：仓库内改为相对/环境变量，适配其他开发者

**目标**：清除仓库内与特定用户、特定盘符绑定的绝对路径，使新开发者只需设置自己的 SDK/OpenOCD
环境变量即可构建。

**改动**：
- `app/CMakeLists.txt`：ESP OpenOCD 不再写死 `C:/Users/12968399/...`；优先
  `$ENV{ESPRESSIF_TOOLCHAIN_PATH}`，否则回退 `$ENV{USERPROFILE}/.zinstaller/tools/openocds`。
- `app/build_all.ps1`：删除 SDK / venv / OpenOCD / ZEPHYR_BASE 的重复硬编码，改为
  dot-source `scripts\env.ps1`（单点维护）。
- `scripts/bootstrap.ps1`：7-Zip 改用 `$env:ProgramFiles`；SDK 改用 `ZEPHYR_SDK_INSTALL_DIR`。
- `zephyr_platform.code-workspace`：SDK 改为 `${env:ZEPHYR_SDK_INSTALL_DIR}`。
- `.trae/skills/zephyr-workbench-env/SKILL.md`：命令与链接去掉旧工作区 `f:\src\zephyr_project`，
  改为工作区根相对路径。
- `docs/development-sop.md`：新增 3.5「绝对路径清单与可移植方案」，并更新 3.3/3.4/4.1 中已失效的
  openair/west manifest 与绝对 SDK 说明。

**关键决策**：外部工具（Zephyr SDK、OpenOCD）无法相对化，统一用环境变量注入；仓库内路径
（app、platform、modules、build）一律相对工作区根。`%USERPROFILE%\.zinstaller\env.ps1` 是机器级
文件，不纳入仓库。

**验证**：grep 排除 `zephyr/modules/.venv/app/build` 后，仓库内已无当前用户名绝对路径；
SDK/OpenOCD 的机器默认值仅保留在 `scripts/env.ps1`、`bootstrap.ps1` 的回退分支与文档说明中。

**遗留**：`app/.vscode/settings.json` 仍为本机 SDK 绝对路径（未入库）；新开发者需在 Workbench 面板
选择本机 SDK。

---

## 2026-10-27 · 修复 atm3405_custom 构建时 SoC 'ATM34xx-5' 未找到

**目标**：Workbench / 普通 `west build` 直接构建 atm3405_custom 时不再报
`SoC 'ATM34xx-5' is not found ... soc-root containing 'ATM34xx-5' has been correctly defined`。

**根因**：本机 `.west/config` 的 `[manifest] path = zephyr`，即 west 使用 `zephyr/west.yml` 作为清单，
根 `west.yml` 中声明的 `atmosic`（openair）项目不会被 `west list` 发现。atm3405 的 SoC 定义
（`modules/hal/atmosic/soc/atmosic/soc.yml`）因此不在 soc_root 中；`platform` 模块因走
`ZEPHYR_EXTRA_MODULES` 仍能被发现，所以表现为"板找到了、SoC 找不到"。

**改动**：
- `app/CMakeLists.txt`：在 `find_package(Zephyr)` 前，仅当 `BOARD == atm3405_custom` 时把
  `modules/hal/atmosic` 加入 `ZEPHYR_EXTRA_MODULES`（与 `app/build_all.ps1` 按板卡拆分的策略一致），
  修正原有"atmosic 已由 west manifest 管理"的失效注释。
- `app/CMakeLists.txt`：`find_package(Zephyr ... HINTS)` 增加工作区 `../zephyr` 回退路径。
  原命令只加载 Workbench 的 `.zinstaller\env.ps1`，此前靠旧构建目录里缓存的 `Zephyr_DIR`
  才能找到包；`--pristine` 清空缓存后即报
  `Could not find a package configuration file provided by "Zephyr"`。
- `%USERPROFILE%\.zinstaller\env.ps1`（仓库外，Workbench 插件实际加载的脚本）：仍指向重命名前的
  `F:\src\zephyr_project`。已更新 `$proj_venv`、`ZEPHYR_BASE`、`ZEPHYR_EXTRA_MODULES`
  （改为 `modules\uwb\uwbiot;platform`；atmosic 由 app/CMakeLists 按板卡挂接）。
  ZEPHYR_BASE 指向旧路径时，`include(zephyr_default)` 会失败，并连带退回宿主 MSYS2 GCC。
- 删除已被旧路径污染的 `app\build\atm3405_custom`，避免 `--pristine never` 复用缓存里的
  `ZEPHYR_BASE:F:/src/zephyr_project/zephyr` 与 `Zephyr_DIR`。

**验证**：本次会话 Shell 工具间歇性无法启动进程，未能完成构建复跑；环境脚本与缓存已修正，
需在 IDE 点击构建或重新执行命令确认。

**遗留**：`.west/config` 与根 `west.yml` 的清单级设计仍不一致；若后续改为 `west init -l .`
（使用根清单），可移除本处的按板卡挂接。

---

## 2026-10-26 · 工作区重命名为 multi-platform-zephyr-project，全面去绝对路径

**目标**：项目根目录由 `zephyr_project` 更名为 `multi-platform-zephyr-project`（用户脚本执行），
清理全部残留绝对路径，保证仓库整体可移动/改名。

**改动**：
- 用户脚本完成：目录改名；`app/build_all.ps1` 改为 `$PSScriptRoot` 推导工作区根（与文件夹名解耦）；docs 路径批量替换
- 本次补齐：
  - `app/debug/jlink_fastload_program.jlink`：6 处旧绝对路径（含已失效的 openair 根目录布局）→ 工作区根相对路径，
    并在头部注明"先 cd 到工作区根再运行 JLink"；fast_load.bin 指向 `modules/hal/atmosic/modules/hal_atmosic/ATM34xx/rev-5/openocd/fast_load/fast_load.bin`
  - `docs/development-sop.md`：7 处绝对路径 → `<工作区根>` 占位 / `$PWD` 相对写法
  - 用户级环境变量 `ZEPHYR_EXTRA_MODULES`（HKCU）：更新为 `<工作区根>\modules\uwb\uwbiot;<工作区根>\platform` 对应的新绝对路径

**关键决策 / 踩坑**：
- 仓库内部一律相对路径；机器级路径（`E:\zephyr-sdk`、`C:\Users\<user>\.zinstaller`）不属于仓库，保持绝对
- 环境变量/注册表是进程上下文，无法相对化，保持绝对；文档与脚本内的工程路径一律相对
- 验证方式：全树 grep `F:\src\`（排除 zephyr/modules/.venv/build）零命中
- **重命名副作用：.venv 启动器硬编码路径**：`.venv\Scripts\west.exe` / `cmake.exe` 是 Python venv 的 launcher stub，
  内部硬编码了创建时的绝对路径（`f:\src\zephyr_project\.venv`），重命名后 west 报 "Not a file: F:/src/zephyr_project/zephyr/cmake/pristine.cmake"。
  处理：`python -m venv .venv --upgrade` 重建启动器，再 `pip install --force-reinstall west==1.5.0 cmake==4.4.4`。
  以后再 rename 工作区根目录时，这一步不能省。
- **HKCU ZEPHYR_EXTRA_MODULES 同步**：注册表内有旧路径，已更新为新绝对路径作为 CLI 兜底

**验证**：三平台重命名后全量构建通过（源码/配置未变，内存占用与重命名前基线完全一致）。

| 板卡 | FLASH | 占比 | RAM | 占比 |
|---|---|---|---|---|
| esp32c6_devkitc | 420,804 B | 5.02% | 189,088 B (sram0) | 37.12% |
| stm32f407ve_custom | 116,588 B | 22.24% | 30,560 B | 23.32% |
| atm3405_custom | 88,652 B | 17.11% | 28,080 B | 10.71% |

**遗留**：旧 `app/build/*` 缓存含旧绝对路径，重命名后首建必须 pristine（`build_all.ps1` 已固定 `-p always`）。

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
