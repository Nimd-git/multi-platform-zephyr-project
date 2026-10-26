/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：系统服务层 -> 虚拟文件系统（LittleFS）
 *
 * 对上游 LittleFS 文件系统的平台统一封装。
 * 业务代码只依赖 plat_fs_* 接口，不直接调用 Zephyr FS API。
 */
#ifndef PLATFORM_PLAT_FS_H_
#define PLATFORM_PLAT_FS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * 初始化并挂载 LittleFS 文件系统
 * @return 0 成功，<0 错误码
 * @note 挂载点由设备树 fstab 定义，通常为 /lfs
 */
int plat_fs_mount(void);

/**
 * 卸载文件系统
 */
int plat_fs_unmount(void);

/**
 * 检查文件系统是否已挂载
 */
bool plat_fs_is_mounted(void);

/**
 * 写文件（覆盖写）
 * @param path 文件路径（如 "/lfs/config.txt"）
 * @param data 数据指针
 * @param len  数据长度
 * @return 0 成功，<0 错误码
 */
int plat_fs_write_file(const char *path, const void *data, size_t len);

/**
 * 读文件
 * @param path    文件路径
 * @param buf     接收缓冲区
 * @param max_len 缓冲区大小
 * @return 实际读取字节数，<0 错误码
 */
int plat_fs_read_file(const char *path, void *buf, size_t max_len);

/**
 * 追加写文件
 * @return 0 成功，<0 错误码
 */
int plat_fs_append_file(const char *path, const void *data, size_t len);

/**
 * 删除文件
 * @return 0 成功，<0 错误码
 */
int plat_fs_delete_file(const char *path);

/**
 * 检查文件是否存在
 */
bool plat_fs_file_exists(const char *path);

/**
 * 获取文件大小
 * @return 文件大小（字节），<0 错误码
 */
int plat_fs_file_size(const char *path);

#endif /* PLATFORM_PLAT_FS_H_ */
