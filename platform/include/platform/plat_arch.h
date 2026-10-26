/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：硬件抽象层 -> 架构移植层
 *   ARM Cortex-M / RISC-V / ARC / x86 的编译期隔离。
 * 平台代码使用 PLAT_ARCH_* 宏而非直接判断 CONFIG_CPU_CORTEX_M/CONFIG_RISCV，
 * 以便后续新增架构时只改本文件。
 */
#ifndef PLATFORM_PLAT_ARCH_H_
#define PLATFORM_PLAT_ARCH_H_

#if defined(CONFIG_CPU_CORTEX_M)
#define PLAT_ARCH_CORTEX_M 1
#define PLAT_ARCH_NAME      "ARM Cortex-M"
#elif defined(CONFIG_CPU_CORTEX_R)
#define PLAT_ARCH_CORTEX_R 1
#define PLAT_ARCH_NAME      "ARM Cortex-R"
#elif defined(CONFIG_CPU_CORTEX_A)
#define PLAT_ARCH_CORTEX_A 1
#define PLAT_ARCH_NAME      "ARM Cortex-A"
#elif defined(CONFIG_RISCV)
#define PLAT_ARCH_RISCV 1
#define PLAT_ARCH_NAME   "RISC-V"
#elif defined(CONFIG_ARC)
#define PLAT_ARCH_ARC 1
#define PLAT_ARCH_NAME "ARC"
#elif defined(CONFIG_X86)
#define PLAT_ARCH_X86 1
#define PLAT_ARCH_NAME "x86"
#else
#define PLAT_ARCH_UNKNOWN 1
#define PLAT_ARCH_NAME    "unknown"
#endif

#if defined(CONFIG_64BIT)
#define PLAT_ARCH_BITS 64
#else
#define PLAT_ARCH_BITS 32
#endif

#endif /* PLATFORM_PLAT_ARCH_H_ */
