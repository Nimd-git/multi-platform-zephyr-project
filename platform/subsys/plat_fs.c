// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：系统服务层 -> 虚拟文件系统（LittleFS）
 * 对上游 LittleFS 文件系统的平台统一封装。
 */

#include <zephyr/kernel.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <string.h>

#include <platform/plat_fs.h>

LOG_MODULE_REGISTER(plat_fs);

#if defined(CONFIG_FILE_SYSTEM_LITTLEFS) && defined(CONFIG_FILE_SYSTEM_LIB_LINK)

/* 默认挂载点（与设备树 fstab 对应） */
#define PLAT_FS_MOUNT_POINT "/lfs"

static bool fs_mounted;

int plat_fs_mount(void)
{
	int ret;

	if (fs_mounted) {
		return 0;
	}

	/* 由设备树 fstab 定义的 littlefs 分区会自动挂载（FS_LITTLEFS_FSTAB_AUTOMOUNT）。
	 * 此处仅检测挂载点是否可用。 */
	struct fs_statvfs stat;

	ret = fs_statvfs(PLAT_FS_MOUNT_POINT, &stat);
	if (ret != 0) {
		LOG_ERR("LittleFS mount point %s not available: %d",
			PLAT_FS_MOUNT_POINT, ret);
		return ret;
	}

	fs_mounted = true;
	LOG_INF("LittleFS mounted at %s (bsize=%lu, blocks=%lu)",
		PLAT_FS_MOUNT_POINT, stat.f_bsize, stat.f_blocks);
	return 0;
}

int plat_fs_unmount(void)
{
	if (!fs_mounted) {
		return 0;
	}

	/* Zephyr 的 fstab automount 不支持运行时卸载，此处仅更新状态 */
	fs_mounted = false;
	LOG_INF("LittleFS unmounted");
	return 0;
}

bool plat_fs_is_mounted(void)
{
	return fs_mounted;
}

int plat_fs_write_file(const char *path, const void *data, size_t len)
{
	struct fs_file_t file;
	int ret;

	if ((path == NULL) || (data == NULL)) {
		return -EINVAL;
	}
	if (!fs_mounted) {
		return -ENODEV;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("fs_open %s failed: %d", path, ret);
		return ret;
	}

	ret = fs_write(&file, data, len);
	if (ret < 0) {
		LOG_ERR("fs_write %s failed: %d", path, ret);
		(void)fs_close(&file);
		return ret;
	}

	ret = fs_close(&file);
	return ret;
}

int plat_fs_read_file(const char *path, void *buf, size_t max_len)
{
	struct fs_file_t file;
	int ret;

	if ((path == NULL) || (buf == NULL)) {
		return -EINVAL;
	}
	if (!fs_mounted) {
		return -ENODEV;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_READ);
	if (ret != 0) {
		return ret;
	}

	ret = fs_read(&file, buf, max_len);
	(void)fs_close(&file);
	return ret;
}

int plat_fs_append_file(const char *path, const void *data, size_t len)
{
	struct fs_file_t file;
	int ret;

	if ((path == NULL) || (data == NULL)) {
		return -EINVAL;
	}
	if (!fs_mounted) {
		return -ENODEV;
	}

	fs_file_t_init(&file);
	ret = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE | FS_O_APPEND);
	if (ret != 0) {
		return ret;
	}

	ret = fs_write(&file, data, len);
	if (ret < 0) {
		(void)fs_close(&file);
		return ret;
	}

	ret = fs_close(&file);
	return ret;
}

int plat_fs_delete_file(const char *path)
{
	if (path == NULL) {
		return -EINVAL;
	}
	if (!fs_mounted) {
		return -ENODEV;
	}

	return fs_unlink(path);
}

bool plat_fs_file_exists(const char *path)
{
	struct fs_dirent entry;

	if ((path == NULL) || !fs_mounted) {
		return false;
	}

	return (fs_stat(path, &entry) == 0) && (entry.type == FS_DIR_ENTRY_FILE);
}

int plat_fs_file_size(const char *path)
{
	struct fs_dirent entry;

	if ((path == NULL) || !fs_mounted) {
		return -ENODEV;
	}

	if (fs_stat(path, &entry) != 0) {
		return -ENOENT;
	}

	return (int)entry.size;
}

#else /* !CONFIG_FILE_SYSTEM_LITTLEFS || !CONFIG_FILE_SYSTEM_LIB_LINK */

/* LittleFS 未启用时的桩实现 */

int plat_fs_mount(void) { return -ENOTSUP; }
int plat_fs_unmount(void) { return -ENOTSUP; }
bool plat_fs_is_mounted(void) { return false; }
int plat_fs_write_file(const char *path, const void *data, size_t len)
{
	ARG_UNUSED(path); ARG_UNUSED(data); ARG_UNUSED(len); return -ENOTSUP;
}
int plat_fs_read_file(const char *path, void *buf, size_t max_len)
{
	ARG_UNUSED(path); ARG_UNUSED(buf); ARG_UNUSED(max_len); return -ENOTSUP;
}
int plat_fs_append_file(const char *path, const void *data, size_t len)
{
	ARG_UNUSED(path); ARG_UNUSED(data); ARG_UNUSED(len); return -ENOTSUP;
}
int plat_fs_delete_file(const char *path)
{
	ARG_UNUSED(path); return -ENOTSUP;
}
bool plat_fs_file_exists(const char *path)
{
	ARG_UNUSED(path); return false;
}
int plat_fs_file_size(const char *path)
{
	ARG_UNUSED(path); return -ENOTSUP;
}

#endif /* CONFIG_FILE_SYSTEM_LITTLEFS && CONFIG_FILE_SYSTEM_LIB_LINK */
