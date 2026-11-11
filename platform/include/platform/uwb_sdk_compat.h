/*
 * nxp-uwbiot SDK 在 Zephyr 下的兼容垫片头（不改动厂商 SDK：modules/ 只读）。
 *
 * 问题：SDK 的 halimpl 源码 phNxpUwb_Common.c 在 FreeRTOS 分支经 cmsis_gcc.h
 * 提供 CMSIS 的 __NOP()；Zephyr 分支未包含任何提供 __NOP 的头，而 Zephyr 构建
 * 开启 -Werror=implicit-int，于是报 "implicit declaration of function '__NOP'"。
 *
 * 注意：CMake 会丢弃"函数式"宏定义（__NOP()=arch_nop() 无法通过 -D 传递），
 * 因此采用强制包含本头的方式（与 modules/hal/espressif 的 zephyr_compat.h 同机制）。
 * 经 zephyr_interface 注入 INTERFACE 编译选项，传播到 halimpl 等所有链接目标。
 *
 * arch_nop() 由 <zephyr/arch/cpu.h> 提供，Zephyr 各架构均有实现。
 */
#ifndef UWBS_SDK_COMPAT_H
#define UWBS_SDK_COMPAT_H

#include <zephyr/arch/cpu.h>

#ifndef __NOP
#define __NOP() arch_nop()
#endif

#endif /* UWBS_SDK_COMPAT_H */
