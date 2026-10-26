/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：内核服务层 -> 系统时钟 / 时间同步
 * 对内核时钟服务的薄封装，统一平台内的时间获取与延时口径。
 */
#ifndef PLATFORM_PLAT_CLOCK_H_
#define PLATFORM_PLAT_CLOCK_H_

#include <stdint.h>

/** 系统启动以来的运行时间（毫秒，64 位，不回绕） */
uint64_t plat_clock_uptime_ms(void);

/** 系统启动以来的运行时间（毫秒，32 位约 49 天回绕；仅用于短时逻辑） */
uint32_t plat_clock_uptime_ms32(void);

/** 阻塞式毫秒延时（线程上下文使用） */
void plat_clock_delay_ms(uint32_t ms);

/** 硬件周期计数器原始值（性能分析/计时基准） */
uint64_t plat_clock_cycles(void);

#endif /* PLATFORM_PLAT_CLOCK_H_ */
