// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：系统服务层 -> 设备管理服务
 *   设备枚举与发现 / 设备健康监测 / 错误计数
 *
 * 上游 Zephyr 已提供设备树解析、设备初始化流程与电源管理（device.h / pm），
 * 本文件在其上补充平台侧的枚举视图与健康计数，不重复造轮子。
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/logging/log.h>

#include <platform/plat_device.h>
#include <platform/plat_health.h>

LOG_MODULE_REGISTER(plat_device_mgmt);

/* ---- 设备枚举与发现 ---------------------------------------------------- */

int plat_device_foreach(plat_device_cb_t cb, void *user_data)
{
	unsigned int index = 0;
	int processed = 0;

	if (cb == NULL) {
		return -EINVAL;
	}

	STRUCT_SECTION_FOREACH(device, dev) {
		int rc = cb(dev, index, user_data);

		processed++;
		if (rc != 0) {
			break;
		}
		index++;
	}

	return processed;
}

int plat_device_count(void)
{
	int count = 0;

	STRUCT_SECTION_FOREACH(device, dev) {
		count++;
	}
	return count;
}

int plat_device_ready_count(void)
{
	int count = 0;

	STRUCT_SECTION_FOREACH(device, dev) {
		if (device_is_ready(dev)) {
			count++;
		}
	}
	return count;
}

/* ---- 设备健康监测 / 错误计数、复位、日志 -------------------------------- */

static atomic_t health_errors;
static volatile int last_error_code;
static const char *volatile last_error_module;

void plat_health_report_error(const char *module, int code)
{
	atomic_inc(&health_errors);
	last_error_code = code;
	last_error_module = (module != NULL) ? module : "unknown";

	if (IS_ENABLED(CONFIG_LOG)) {
		LOG_WRN("health error: module=%s code=%d total=%u",
			last_error_module, code, (uint32_t)atomic_get(&health_errors));
	}
}

uint32_t plat_health_error_count(void)
{
	return (uint32_t)atomic_get(&health_errors);
}

const char *plat_health_last_module(void)
{
	return last_error_module;
}

int plat_health_last_code(void)
{
	return last_error_code;
}
