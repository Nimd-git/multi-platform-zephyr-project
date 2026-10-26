/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：应用开发层 -> 存储中间件（键值存储/数据校验）
 *
 * 上游 settings 子系统的平台封装：应用只用 "模块/键" 形式的字符串访问，
 * 后端可以是 RAM（QEMU 冒烟）、FCB/Flash 或 LittleFS 文件，切换后端无需
 * 修改应用代码。
 */
#ifndef PLATFORM_PLAT_KV_H_
#define PLATFORM_PLAT_KV_H_

#include <stdint.h>
#include <stddef.h>

/** 单条 KV 值的最大长度（字节） */
#define PLAT_KV_MAX_VAL_LEN 64

/** 初始化 KV 服务并加载已保存内容（平台核心自动调用一次） */
int plat_kv_init(void);

/** 保存一个 32 位无符号整数（完整名如 "plat/boot"） */
int plat_kv_set_u32(const char *full_key, uint32_t value);

/** 读取一个 32 位无符号整数；不存在时返回 -ENOENT */
int plat_kv_get_u32(const char *full_key, uint32_t *value);

/** 保存一段二进制数据（完整名 "plat/xxx"，最长 PLAT_KV_MAX_VAL_LEN 字节） */
int plat_kv_set_blob(const char *full_key, const void *data, size_t len);

/** 读取一段二进制数据；out_len 可为 NULL；不存在时返回 -ENOENT */
int plat_kv_get_blob(const char *full_key, void *buf, size_t buf_len, size_t *out_len);

/** 删除一个键 */
int plat_kv_delete(const char *full_key);

/** 提交挂起的持久化操作（RAM 后端下为空操作） */
int plat_kv_commit(void);

/** 启动计数（每次上电加 1；RAM 后端下仅用于演示接口链路） */
uint32_t plat_kv_boot_count(void);

#endif /* PLATFORM_PLAT_KV_H_ */
