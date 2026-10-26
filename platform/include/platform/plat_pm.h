/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：系统服务层 -> 设备管理服务 -> 运行时电源管理
 *
 * 对上游 PM device runtime API 的平台统一封装。
 * 业务代码只依赖 plat_pm_* 接口，不直接调用 Zephyr PM API。
 */
#ifndef PLATFORM_PLAT_PM_H_
#define PLATFORM_PLAT_PM_H_

#include <stdbool.h>
#include <zephyr/device.h>

/**
 * 启用设备的运行时电源管理
 * @param dev 设备指针
 * @return 0 成功，<0 错误码
 * @note 启用后设备默认进入 suspended 状态，需通过 plat_pm_get 唤醒
 */
int plat_pm_enable(const struct device *dev);

/**
 * 禁用设备的运行时电源管理
 * @param dev 设备指针
 * @return 0 成功，<0 错误码
 */
int plat_pm_disable(const struct device *dev);

/**
 * 获取设备（唤醒并增加引用计数）
 * @param dev 设备指针
 * @return 0 成功，<0 错误码
 * @note 每次 get 必须配对一次 put
 */
int plat_pm_get(const struct device *dev);

/**
 * 释放设备（减少引用计数，计数为 0 时自动休眠）
 * @param dev 设备指针
 * @return 0 成功，<0 错误码
 */
int plat_pm_put(const struct device *dev);

/**
 * 异步释放设备（延迟休眠，适用于 ISR 上下文）
 * @param dev   设备指针
 * @param delay 延迟时间（如 K_MSEC(100)）
 * @return 0 成功，<0 错误码
 */
int plat_pm_put_async(const struct device *dev, k_timeout_t delay);

/**
 * 查询设备运行时 PM 是否已启用
 * @return true=已启用，false=未启用
 */
bool plat_pm_is_enabled(const struct device *dev);

/**
 * 查询设备当前引用计数
 * @return 引用计数，<0 错误码
 */
int plat_pm_usage_count(const struct device *dev);

#endif /* PLATFORM_PLAT_PM_H_ */
