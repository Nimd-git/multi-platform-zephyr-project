/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 自研骨架驱动 plat,platform-monitor 的公共 API。
 * 演示统一设备模型下“自定义驱动 API + 设备树实例化”的标准做法。
 */
#ifndef PLATFORM_PLAT_MONITOR_H_
#define PLATFORM_PLAT_MONITOR_H_

#include <zephyr/device.h>
#include <errno.h>
#include <stdint.h>

/** 骨架驱动自定义 API：读取驱动内计数与时间戳 */
struct plat_monitor_api {
	/** 返回 0 成功；uptime_ms 输出调用时刻的系统毫秒数 */
	int (*ping)(const struct device *dev, uint32_t *uptime_ms);
};

/** 调用 monitor 设备的 ping 操作 */
static inline int plat_monitor_ping(const struct device *dev, uint32_t *uptime_ms)
{
	const struct plat_monitor_api *api =
		(const struct plat_monitor_api *)dev->api;

	if ((api == NULL) || (api->ping == NULL)) {
		return -ENOTSUP;
	}
	return api->ping(dev, uptime_ms);
}

#endif /* PLATFORM_PLAT_MONITOR_H_ */
