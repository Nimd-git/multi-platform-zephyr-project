// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：应用开发层 -> 应用框架 -> 应用状态机 / 资源回收与异常处理
 *
 * 状态流：
 *   INIT --(init)------> SELFTEST --(selftest pass, 2s)--> RUNNING
 *                              RUNNING --(fault)--> DEGRADED
 *                              DEGRADED --(recover, 3s)--> RUNNING
 *
 * 异步迁移由系统工作队列（k_work_delayable）驱动；主循环周期性心跳由
 * app_fsm_tick() 驱动，体现“主循环调度”。
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>

#include "app_state_machine.h"

LOG_MODULE_REGISTER(app_fsm);

#define SELFTEST_DELAY_MS  2000U
#define RECOVER_DELAY_MS   3000U
#define HEARTBEAT_PERIOD_MS 5000U

static enum app_fsm_state current;
static struct k_work_delayable selftest_work;
static struct k_work_delayable recover_work;
static uint32_t last_heartbeat_ms;

static const char *const state_names[] = {
	[APP_STATE_INIT] = "INIT",
	[APP_STATE_SELFTEST] = "SELFTEST",
	[APP_STATE_RUNNING] = "RUNNING",
	[APP_STATE_DEGRADED] = "DEGRADED",
};

static void transition(enum app_fsm_state next)
{
	if (next == current) {
		return;
	}
	LOG_INF("[fsm] %s -> %s", state_names[current], state_names[next]);
	current = next;
}

static void selftest_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	app_fsm_dispatch(APP_EV_SELFTEST_PASS);
}

static void recover_handler(struct k_work *work)
{
	ARG_UNUSED(work);
	app_fsm_dispatch(APP_EV_RECOVER);
}

void app_fsm_init(void)
{
	current = APP_STATE_INIT;
	k_work_init_delayable(&selftest_work, selftest_handler);
	k_work_init_delayable(&recover_work, recover_handler);

	transition(APP_STATE_SELFTEST);
	(void)k_work_schedule(&selftest_work, K_MSEC(SELFTEST_DELAY_MS));
}

void app_fsm_dispatch(enum app_fsm_event event)
{
	switch (current) {
	case APP_STATE_SELFTEST:
		if (event == APP_EV_SELFTEST_PASS) {
			transition(APP_STATE_RUNNING);
			last_heartbeat_ms = k_uptime_get_32();
		}
		break;
	case APP_STATE_RUNNING:
		if (event == APP_EV_FAULT) {
			transition(APP_STATE_DEGRADED);
			(void)k_work_schedule(&recover_work,
					      K_MSEC(RECOVER_DELAY_MS));
		}
		break;
	case APP_STATE_DEGRADED:
		if (event == APP_EV_RECOVER) {
			transition(APP_STATE_RUNNING);
			last_heartbeat_ms = k_uptime_get_32();
		}
		break;
	default:
		LOG_WRN("[fsm] event %d ignored in state %s", event,
			state_names[current]);
		break;
	}
}

void app_fsm_tick(void)
{
	if (current != APP_STATE_RUNNING) {
		return;
	}
	if ((k_uptime_get_32() - last_heartbeat_ms) < HEARTBEAT_PERIOD_MS) {
		return;
	}
	last_heartbeat_ms = k_uptime_get_32();
	LOG_INF("[fsm] heartbeat, state=RUNNING, uptime=%u ms",
		k_uptime_get_32());
}

enum app_fsm_state app_fsm_state(void)
{
	return current;
}

const char *app_fsm_state_name(void)
{
	return state_names[current];
}
