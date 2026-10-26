/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：系统服务层 -> 设备管理服务
 *   设备健康监测 / 错误计数、复位、日志
 */
#ifndef PLATFORM_PLAT_HEALTH_H_
#define PLATFORM_PLAT_HEALTH_H_

#include <stdint.h>

/** 上报一次模块错误：累计计数并输出告警日志（可在中断上下文调用） */
void plat_health_report_error(const char *module, int code);

/** 自启动以来的错误总数 */
uint32_t plat_health_error_count(void);

/** 最后一次错误的来源模块名（未发生过错误时返回 NULL） */
const char *plat_health_last_module(void);

/** 最后一次错误码（未发生过错误时返回 0） */
int plat_health_last_code(void);

#endif /* PLATFORM_PLAT_HEALTH_H_ */
