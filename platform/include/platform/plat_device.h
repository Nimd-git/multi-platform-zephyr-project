/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：系统服务层 -> 设备管理服务
 *           硬件抽象层 -> 统一设备模型（设备树解析/设备初始化流程）
 */
#ifndef PLATFORM_PLAT_DEVICE_H_
#define PLATFORM_PLAT_DEVICE_H_

#include <stdbool.h>
#include <zephyr/device.h>

/** 设备遍历回调：index 为序号；返回非 0 可提前终止遍历 */
typedef int (*plat_device_cb_t)(const struct device *dev, unsigned int index,
				void *user_data);

/** 遍历内核设备表中的全部设备（含未就绪），返回遍历到的设备数 */
int plat_device_foreach(plat_device_cb_t cb, void *user_data);

/** 设备总数 / 当前已就绪设备数 */
int plat_device_count(void);
int plat_device_ready_count(void);

#endif /* PLATFORM_PLAT_DEVICE_H_ */
