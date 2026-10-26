// SPDX-License-Identifier: Apache-2.0
/*
 * 平台框架双架构冒烟应用
 *
 * 架构图对应：
 *   硬件抽象层  ：plat_gpio 统一封装 + 自研 monitor 驱动 + 双架构 overlay
 *   内核服务层  ：线程/消息队列/事件/定时器/workqueue（app_kernel_demo）
 *   系统服务层  ：设备枚举、健康监测、日志、shell（platform 模块）
 *                + LittleFS / 网络 / PM（app_services_demo）
 *   应用开发层  ：应用生命周期、应用状态机、主循环调度、shell 命令
 *   版本兼容柱  ：plat_version.h / Kconfig.compat / check_version.py
 */

#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <zephyr/shell/shell.h>

#include <platform/plat_arch.h>

#include "app_lifecycle.h"
#include "app_state_machine.h"
#include "app_kernel_demo.h"
#include "app_services_demo.h"
#include "app_middleware_demo.h"

LOG_MODULE_REGISTER(app);

/* ---- 应用层 shell 命令：app state / app fault -------------------------- */

static int cmd_app_state(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "fsm state: %s", app_fsm_state_name());
	return 0;
}

static int cmd_app_fault(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	shell_print(sh, "injecting fault event");
	app_fsm_dispatch(APP_EV_FAULT);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	app_sub,
	SHELL_CMD(state, NULL, "Show application FSM state", cmd_app_state),
	SHELL_CMD(fault, NULL, "Inject a fault into the FSM", cmd_app_fault),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(app, &app_sub, "Smoke application commands", NULL);

/* ---- 应用入口与主循环调度 ---------------------------------------------- */

#define MAIN_LOOP_PERIOD_MS 100U

int main(void)
{
	app_lifecycle_boot_complete();
	app_fsm_init();
	app_kernel_demo_start();
	app_services_demo_start();
	app_middleware_demo_start();

	LOG_INF("Platform smoke started | arch=%s | zephyr=%s",
		PLAT_ARCH_NAME, KERNEL_VERSION_STRING);

	/* 架构图：应用框架 -> 主循环调度 */
	while (1) {
		app_fsm_tick();
		k_msleep(MAIN_LOOP_PERIOD_MS);
	}

	return 0;
}
