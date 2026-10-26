// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：应用开发层 -> 应用框架
 *   应用生命周期管理 / 资源回收与异常处理
 *
 * 演示 Zephyr 分阶段初始化流水线中应用钩子的挂载方式：
 *   EARLY -> PRE_KERNEL_1/2 -> POST_KERNEL -> APPLICATION -> main()
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "app_lifecycle.h"

LOG_MODULE_REGISTER(app_lifecycle);

/* POST_KERNEL：硬件与平台设备初始化完成，进入应用准备阶段 */
static int app_lifecycle_post_kernel(void)
{
	LOG_INF("[lifecycle] POST_KERNEL: drivers ready, app preparing");
	return 0;
}
SYS_INIT(app_lifecycle_post_kernel, POST_KERNEL, 60);

/* APPLICATION（早于平台核心 40 之外的其他服务按需排序） */
static int app_lifecycle_application(void)
{
	LOG_INF("[lifecycle] APPLICATION: services initialized");
	return 0;
}
SYS_INIT(app_lifecycle_application, APPLICATION, 5);

void app_lifecycle_boot_complete(void)
{
	LOG_INF("[lifecycle] main entered, application is RUNNING");
}
