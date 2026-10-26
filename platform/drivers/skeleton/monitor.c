// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：硬件抽象层 -> 统一设备模型 / 驱动 API 标准化
 *
 * 自研骨架驱动：plat,platform-monitor
 * 纯软件设备，无真实硬件依赖，用于演示并验证下列机制在 Cortex-M 与
 * RISC-V 上的完整链路：
 *   1. 设备树节点 + 自定义 binding（platform/dts/bindings/）
 *   2. DT_INST_* 自动实例化
 *   3. DEVICE_DT_INST_DEFINE 注册到统一设备模型（POST_KERNEL 初始化级别）
 *   4. 自定义驱动 API（struct plat_monitor_api）
 *
 * QEMU 板卡在 app/boards 目录下的 .overlay 文件中各定义了一个该节点。
 * 后续真实板驱动（传感器/通信芯片）按此模板替换 compatible 与操作集即可。
 */

#define DT_DRV_COMPAT plat_platform_monitor

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>

#include <platform/plat_monitor.h>

LOG_MODULE_REGISTER(plat_monitor);

struct monitor_data {
	uint32_t boot_ms;   /* 驱动初始化时刻 */
	uint32_t pings;     /* ping 调用次数 */
};

struct monitor_config {
	uint32_t report_interval_ms;    /* 来自设备树 report-interval-ms */
};

static int monitor_ping(const struct device *dev, uint32_t *uptime_ms)
{
	struct monitor_data *data = dev->data;

	data->pings++;
	if (uptime_ms != NULL) {
		*uptime_ms = k_uptime_get_32() - data->boot_ms;
	}
	return 0;
}

static const struct plat_monitor_api monitor_api = {
	.ping = monitor_ping,
};

static int monitor_init(const struct device *dev)
{
	const struct monitor_config *cfg = dev->config;
	struct monitor_data *data = dev->data;

	data->boot_ms = k_uptime_get_32();
	data->pings = 0U;

	LOG_INF("%s ready (report-interval=%u ms)", dev->name,
		cfg->report_interval_ms);
	return 0;
}

#define MONITOR_DEVICE_DEFINE(inst)                                                           \
	static struct monitor_data monitor_data_##inst;                                        \
	static const struct monitor_config monitor_config_##inst = {                          \
		.report_interval_ms = DT_INST_PROP(inst, report_interval_ms),                       \
	};                                                                                     \
	DEVICE_DT_INST_DEFINE(inst, monitor_init, NULL, &monitor_data_##inst,                    \
			      &monitor_config_##inst, POST_KERNEL,                                   \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &monitor_api)

DT_INST_FOREACH_STATUS_OKAY(MONITOR_DEVICE_DEFINE)
