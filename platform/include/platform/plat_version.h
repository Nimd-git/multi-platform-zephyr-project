/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：版本兼容与适配 -> API 兼容层（版本宏隔离/废弃接口兼容）
 *
 * 所有跨 Zephyr 版本的 API 差异都必须经由本头文件的宏隔离，业务代码只允许
 * 使用 PLAT_ZEPHYR_IS_VERSION() / PLAT_ZEPHYR_VERSION_AT_LEAST() 判断版本，
 * 严禁直接在业务代码中引用 KERNEL_VERSION_*。
 */
#ifndef PLATFORM_PLAT_VERSION_H_
#define PLATFORM_PLAT_VERSION_H_

#include <zephyr/version.h>
#include <zephyr/toolchain.h>

/* (major, minor) 二元组版本判断 */
#define PLAT_ZEPHYR_IS_VERSION(maj, min)                                                    \
	((KERNEL_VERSION_MAJOR == (maj)) && (KERNEL_VERSION_MINOR == (min)))

#define PLAT_ZEPHYR_VERSION_AT_LEAST(maj, min)                                               \
	((KERNEL_VERSION_MAJOR > (maj)) ||                                                    \
	 ((KERNEL_VERSION_MAJOR == (maj)) && (KERNEL_VERSION_MINOR >= (min))))

/* 平台层支持基线：v3.7 LTS（向前兼容） */
#if !PLAT_ZEPHYR_VERSION_AT_LEAST(3, 7)
#error "Platform framework requires Zephyr v3.7 LTS or newer"
#endif

/* 废弃接口标记：配合 CONFIG_PLAT_ENABLE_DEPRECATED_API 使用 */
#if defined(CONFIG_PLAT_ENABLE_DEPRECATED_API)
#define PLAT_DEPRECATED_SINCE(maj, min, msg) __DEPRECATED_MACRO(msg)
#else
#define PLAT_DEPRECATED_SINCE(maj, min, msg)
#endif

#endif /* PLATFORM_PLAT_VERSION_H_ */
