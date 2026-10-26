/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：应用开发层 -> 应用框架 -> 应用状态机
 */
#ifndef APP_STATE_MACHINE_H_
#define APP_STATE_MACHINE_H_

enum app_fsm_state {
	APP_STATE_INIT = 0,
	APP_STATE_SELFTEST,
	APP_STATE_RUNNING,
	APP_STATE_DEGRADED,
};

enum app_fsm_event {
	APP_EV_SELFTEST_PASS,
	APP_EV_FAULT,
	APP_EV_RECOVER,
};

void app_fsm_init(void);
void app_fsm_dispatch(enum app_fsm_event event);
void app_fsm_tick(void);
enum app_fsm_state app_fsm_state(void);
const char *app_fsm_state_name(void);

#endif /* APP_STATE_MACHINE_H_ */
