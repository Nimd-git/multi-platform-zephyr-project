// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：应用开发层 -> 存储中间件（键值存储）
 *
 * 基于上游 settings 子系统的统一 KV 封装：
 *   - QEMU 冒烟：CONFIG_SETTINGS_NONE，键值保存在 RAM，用于打通接口链路；
 *   - 真实硬件：切换 SETTINGS_FCB / SETTINGS_FILE（LittleFS/FCB）后，
 *     本文件与应用代码均无需修改即可获得持久化。
 *
 * 通用 KV：内部维护固定槽位注册表（cache），写操作同时更新
 * settings 后端（save_one/delete），读操作直接命中注册表；
 * 上电 settings_load() 时由 handler 将后端记录回填注册表。
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/settings/settings.h>
#include <errno.h>
#include <string.h>

#include <platform/plat_kv.h>

LOG_MODULE_REGISTER(plat_kv);

#define KV_SUBTREE    "plat"
#define KV_BOOT_KEY   "plat/boot"

#define KV_MAX_ENTRIES  16
#define KV_MAX_VAL_LEN  64
#define KV_MAX_KEY_LEN  24

struct kv_entry {
	bool used;
	char key[KV_MAX_KEY_LEN];
	uint8_t val[KV_MAX_VAL_LEN];
	size_t val_len;
};

static struct kv_entry kv_entries[KV_MAX_ENTRIES];

static struct kv_entry *kv_entry_find(const char *key)
{
	for (int i = 0; i < KV_MAX_ENTRIES; i++) {
		if (kv_entries[i].used && (strcmp(kv_entries[i].key, key) == 0)) {
			return &kv_entries[i];
		}
	}
	return NULL;
}

static struct kv_entry *kv_entry_alloc(const char *key)
{
	for (int i = 0; i < KV_MAX_ENTRIES; i++) {
		if (!kv_entries[i].used) {
			kv_entries[i].used = true;
			strncpy(kv_entries[i].key, key, sizeof(kv_entries[i].key) - 1);
			kv_entries[i].key[sizeof(kv_entries[i].key) - 1] = '\0';
			kv_entries[i].val_len = 0;
			return &kv_entries[i];
		}
	}
	return NULL;
}

/* settings 子树 "plat" 的静态处理函数：键名为子树之后的部分 */
static int kv_set(const char *key, size_t len, settings_read_cb read_cb, void *cb_arg)
{
	struct kv_entry *entry;
	size_t copy_len;

	if (key == NULL) {
		return -EINVAL;
	}
	if ((strlen(key) >= KV_MAX_KEY_LEN) || (len > KV_MAX_VAL_LEN)) {
		return -EINVAL;
	}

	entry = kv_entry_find(key);
	if (entry == NULL) {
		entry = kv_entry_alloc(key);
	}
	if (entry == NULL) {
		return -ENOMEM;
	}

	copy_len = (len <= KV_MAX_VAL_LEN) ? len : KV_MAX_VAL_LEN;
	if (copy_len > 0 && read_cb(cb_arg, entry->val, copy_len) != (int)copy_len) {
		return -EIO;
	}
	entry->val_len = copy_len;
	return 0;
}

static int kv_get(const char *key, char *val, int val_len_max)
{
	struct kv_entry *entry;

	entry = kv_entry_find(key);
	if (entry == NULL) {
		return -ENOENT;
	}
	if (val_len_max < (int)entry->val_len) {
		return -ENOMEM;
	}
	memcpy(val, entry->val, entry->val_len);
	return (int)entry->val_len;
}

static int kv_commit(void)
{
	/* 无需额外动作：注册表即最新状态 */
	return 0;
}

static int kv_export(int (*write_func)(const char *name, const void *value, size_t val_len))
{
	for (int i = 0; i < KV_MAX_ENTRIES; i++) {
		if (!kv_entries[i].used || kv_entries[i].val_len == 0) {
			continue;
		}
		char name[32];
		int n = snprintf(name, sizeof(name), KV_SUBTREE "/%s", kv_entries[i].key);

		if (n <= 0 || n >= (int)sizeof(name)) {
			continue;
		}
		(void)write_func(name, kv_entries[i].val, kv_entries[i].val_len);
	}
	return 0;
}

SETTINGS_STATIC_HANDLER_DEFINE(plat, KV_SUBTREE, kv_get, kv_set, kv_commit, kv_export);

int plat_kv_init(void)
{
	int ret;
	uint32_t boot;

	ret = settings_subsys_init();
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("settings_subsys_init failed: %d", ret);
		return ret;
	}

	(void)settings_load();

	/* 启动计数：每次上电加 1 */
	if (plat_kv_get_u32(KV_BOOT_KEY, &boot) != 0) {
		boot = 0;
	}
	boot++;
	(void)plat_kv_set_u32(KV_BOOT_KEY, boot);

	LOG_INF("KV storage ready, boot count=%u (backend=%s)", boot,
		IS_ENABLED(CONFIG_SETTINGS_NONE) ? "RAM" : "persistent");
	return 0;
}

/* APPLICATION 20：早于平台核心（40）与应用初始化 */
SYS_INIT(plat_kv_init, APPLICATION, 20);

int plat_kv_set_blob(const char *full_key, const void *data, size_t len)
{
	int ret;

	if ((full_key == NULL) || (data == NULL) || (len == 0) || (len > KV_MAX_VAL_LEN)) {
		return -EINVAL;
	}

	/* 先更新运行时缓存（走 kv_set handler），再写入后端持久化 */
	ret = settings_runtime_set(full_key, data, len);
	if (ret != 0) {
		return ret;
	}
	ret = settings_save_one(full_key, data, len);
	if (ret == -ENOENT) {
		/* RAM 后端等无持久化场景：缓存已更新即视为成功 */
		return 0;
	}
	return ret;
}

int plat_kv_get_blob(const char *full_key, void *buf, size_t buf_len, size_t *out_len)
{
	struct kv_entry *entry;
	const char *key;

	if ((full_key == NULL) || (buf == NULL)) {
		return -EINVAL;
	}

	/* full_key 形如 "plat/xxx"，handler 键名为子树之后部分 */
	key = strchr(full_key, '/');
	if (key == NULL) {
		return -EINVAL;
	}
	key++;

	entry = kv_entry_find(key);
	if (entry == NULL) {
		return -ENOENT;
	}
	if (buf_len < entry->val_len) {
		return -ENOMEM;
	}
	memcpy(buf, entry->val, entry->val_len);
	if (out_len != NULL) {
		*out_len = entry->val_len;
	}
	return 0;
}

int plat_kv_delete(const char *full_key)
{
	struct kv_entry *entry;
	const char *key;
	int ret;

	if (full_key == NULL) {
		return -EINVAL;
	}

	key = strchr(full_key, '/');
	if (key != NULL) {
		key++;
		entry = kv_entry_find(key);
		if (entry != NULL) {
			entry->used = false;
		}
	}

	ret = settings_delete(full_key);
	if (ret == -ENOENT) {
		return 0;
	}
	return ret;
}

int plat_kv_commit(void)
{
	return settings_commit();
}

int plat_kv_set_u32(const char *full_key, uint32_t value)
{
	return plat_kv_set_blob(full_key, &value, sizeof(value));
}

int plat_kv_get_u32(const char *full_key, uint32_t *value)
{
	size_t len;

	if ((full_key == NULL) || (value == NULL)) {
		return -EINVAL;
	}
	if (plat_kv_get_blob(full_key, value, sizeof(*value), &len) != 0) {
		return -ENOENT;
	}
	return 0;
}

uint32_t plat_kv_boot_count(void)
{
	uint32_t boot = 0;

	(void)plat_kv_get_u32(KV_BOOT_KEY, &boot);
	return boot;
}
