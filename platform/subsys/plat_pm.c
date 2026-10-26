// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：系统服务层 -> 设备管理服务 -> 运行时电源管理
 * 对上游 PM device runtime API 的平台统一封装。
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/pm/device_runtime.h>
#include <errno.h>

#include <platform/plat_pm.h>

int plat_pm_enable(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_enable(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

int plat_pm_disable(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_disable(dev);
#else
	ARG_UNUSED(dev);
	return -ENOTSUP;
#endif
}

int plat_pm_get(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_get(dev);
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

int plat_pm_put(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_put(dev);
#else
	ARG_UNUSED(dev);
	return 0;
#endif
}

int plat_pm_put_async(const struct device *dev, k_timeout_t delay)
{
	if (dev == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_put_async(dev, delay);
#else
	ARG_UNUSED(dev);
	ARG_UNUSED(delay);
	return 0;
#endif
}

bool plat_pm_is_enabled(const struct device *dev)
{
	if (dev == NULL) {
		return false;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_is_enabled(dev);
#else
	ARG_UNUSED(dev);
	return false;
#endif
}

int plat_pm_usage_count(const struct device *dev)
{
	if (dev == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_PM_DEVICE_RUNTIME)
	return pm_device_runtime_usage(dev);
#else
	ARG_UNUSED(dev);
	return -ENOSYS;
#endif
}
