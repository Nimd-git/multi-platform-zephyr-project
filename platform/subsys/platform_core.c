// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：系统服务层（平台核心服务）+ 内核服务层（系统时钟封装）
 *
 * 职责：
 *   - 平台启动横幅与版本/架构上报（APPLICATION 初始化级别演示）
 *   - 统一时钟服务实现（plat_clock.h）
 *   - platform shell 命令集：ver / devices / uptime / led / kv / ping
 */

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <string.h>

#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>
#endif

#include <platform/plat_version.h>
#include <platform/plat_arch.h>
#include <platform/plat_clock.h>
#include <platform/plat_device.h>
#include <platform/plat_kv.h>

#if defined(CONFIG_GPIO)
#include <zephyr/drivers/gpio.h>
#include <platform/plat_gpio.h>
#if DT_NODE_HAS_STATUS(DT_ALIAS(plat_led0), okay)
#define PLAT_HAS_LED 1
static const struct gpio_dt_spec plat_led =
	GPIO_DT_SPEC_GET(DT_ALIAS(plat_led0), gpios);
#else
#define PLAT_HAS_LED 0
#endif
#endif /* CONFIG_GPIO */

#if defined(CONFIG_PLAT_MONITOR)
#include <platform/plat_monitor.h>
#if DT_HAS_COMPAT_STATUS_OKAY(plat_platform_monitor)
#define PLAT_HAS_MONITOR 1
static const struct device *const plat_monitor_dev =
	DEVICE_DT_GET_ONE(plat_platform_monitor);
#else
#define PLAT_HAS_MONITOR 0
#endif
#endif

LOG_MODULE_REGISTER(plat_core);

/* ---- 内核服务层：系统时钟/时间同步封装 ---------------------------------- */

uint64_t plat_clock_uptime_ms(void)
{
	return k_uptime_get();
}

uint32_t plat_clock_uptime_ms32(void)
{
	return k_uptime_get_32();
}

void plat_clock_delay_ms(uint32_t ms)
{
	k_msleep(ms);
}

uint64_t plat_clock_cycles(void)
{
	return k_cycle_get_64();
}

/* ---- 平台启动横幅（应用生命周期 APPLICATION 阶段） ---------------------- */

static int plat_core_init(void)
{
	LOG_INF("==================================================");
	LOG_INF(" Platform Framework on Zephyr %s", KERNEL_VERSION_STRING);
	LOG_INF(" arch=%s (%d-bit)", PLAT_ARCH_NAME, PLAT_ARCH_BITS);
	LOG_INF(" devices: %d total, %d ready",
		plat_device_count(), plat_device_ready_count());
	LOG_INF("==================================================");
	return 0;
}

SYS_INIT(plat_core_init, APPLICATION, 40);

/* ---- platform shell 命令集 --------------------------------------------- */

#if defined(CONFIG_SHELL)

static int cmd_ver(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "Zephyr %s", KERNEL_VERSION_STRING);
	shell_print(sh, "arch  : %s (%d-bit)", PLAT_ARCH_NAME, PLAT_ARCH_BITS);
	shell_print(sh, "kernel: %d.%d.%d (platform baseline >= v3.7 LTS)",
		    KERNEL_VERSION_MAJOR, KERNEL_VERSION_MINOR,
		    KERNEL_PATCHLEVEL);
	return 0;
}

struct device_dump_ctx {
	const struct shell *sh;
};

static int device_dump_cb(const struct device *dev, unsigned int index,
			  void *user_data)
{
	struct device_dump_ctx *ctx = user_data;

	shell_print(ctx->sh, "  [%2u] %-28s %s", index, dev->name,
		    device_is_ready(dev) ? "ready" : "NOT ready");
	return 0;
}

static int cmd_devices(const struct shell *sh, size_t argc, char **argv)
{
	struct device_dump_ctx ctx = { .sh = sh };

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "devices (%d total / %d ready):",
		    plat_device_count(), plat_device_ready_count());
	(void)plat_device_foreach(device_dump_cb, &ctx);
	return 0;
}

static int cmd_uptime(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "uptime: %llu ms",
		    (unsigned long long)plat_clock_uptime_ms());
	shell_print(sh, "cycles: %llu",
		    (unsigned long long)plat_clock_cycles());
	return 0;
}

static int cmd_led(const struct shell *sh, size_t argc, char **argv)
{
#if PLAT_HAS_LED
	int ret;

	if (!gpio_is_ready_dt(&plat_led)) {
		shell_print(sh, "plat-led0 device not ready");
		return -ENODEV;
	}

	if (strcmp(argv[2], "on") == 0) {
		ret = plat_led_set(&plat_led, true);
	} else if (strcmp(argv[2], "off") == 0) {
		ret = plat_led_set(&plat_led, false);
	} else if (strcmp(argv[2], "toggle") == 0) {
		ret = plat_led_toggle(&plat_led);
	} else {
		shell_print(sh, "usage: platform led <on|off|toggle>");
		return -EINVAL;
	}
	shell_print(sh, "led %s -> ret=%d", argv[2], ret);
	return ret;
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);
	shell_print(sh, "plat-led0 not defined in devicetree");
	return -ENOTSUP;
#endif
}

static int cmd_kv(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t value = 0U;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!IS_ENABLED(CONFIG_PLAT_MW_STORAGE)) {
		shell_print(sh, "storage middleware disabled");
		return -ENOTSUP;
	}

	ret = plat_kv_set_u32("plat/demo", 42U);
	if (ret == 0) {
		ret = plat_kv_get_u32("plat/demo", &value);
	}
	shell_print(sh, "KV round-trip: set=42 get=%u ret=%d", value, ret);
	shell_print(sh, "boot count=%u", plat_kv_boot_count());
	return ret;
}

static int cmd_ping(const struct shell *sh, size_t argc, char **argv)
{
	uint32_t since_boot_ms = 0U;
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if PLAT_HAS_MONITOR
	if (!device_is_ready(plat_monitor_dev)) {
		shell_print(sh, "platform-monitor not ready");
		return -ENODEV;
	}
	ret = plat_monitor_ping(plat_monitor_dev, &since_boot_ms);
	shell_print(sh, "pong: monitor uptime=%u ms ret=%d", since_boot_ms, ret);
	return ret;
#else
	shell_print(sh, "platform-monitor node not present");
	return -ENOTSUP;
#endif
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	platform_sub,
	SHELL_CMD(ver, NULL, "Show Zephyr/arch version", cmd_ver),
	SHELL_CMD(devices, NULL, "Enumerate kernel devices", cmd_devices),
	SHELL_CMD(uptime, NULL, "Show uptime and cycle counter", cmd_uptime),
	SHELL_CMD_ARG(led, NULL, "<on|off|toggle> - control plat-led0",
		      cmd_led, 3, 0),
	SHELL_CMD(kv, NULL, "Key-value storage round-trip demo", cmd_kv),
	SHELL_CMD(ping, NULL, "Ping skeleton monitor driver", cmd_ping),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(platform, &platform_sub,
		       "Platform framework commands", NULL, 1, 1);

#endif /* CONFIG_SHELL */
