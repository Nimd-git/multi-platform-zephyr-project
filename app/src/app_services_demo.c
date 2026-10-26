// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：系统服务层 -> 设备管理 / 虚拟文件系统 / 网络协议栈
 *
 * 本文件演示平台系统服务的使用：
 *   - LittleFS 文件读写（plat_fs_*）
 *   - 网络连接 + MQTT 发布/订阅（plat_net_*）
 *   - 设备运行时电源管理（plat_pm_*）
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <platform/plat_fs.h>
#include <platform/plat_net.h>
#include <platform/plat_pm.h>
#include <platform/plat_device.h>

#include "app_services_demo.h"

LOG_MODULE_REGISTER(services_demo);

/* ---- LittleFS 演示 -------------------------------------------------------- */

#if defined(CONFIG_PLAT_FS)

static void demo_filesystem(void)
{
	const char *test_path = "/lfs/platform_test.txt";
	const char *test_data = "Hello from Platform Framework!";
	char read_buf[64];
	int ret;

	ret = plat_fs_mount();
	if (ret != 0) {
		LOG_WRN("LittleFS not available: %d", ret);
		return;
	}

	ret = plat_fs_write_file(test_path, test_data, strlen(test_data));
	if (ret != 0) {
		LOG_ERR("FS write failed: %d", ret);
		return;
	}

	ret = plat_fs_read_file(test_path, read_buf, sizeof(read_buf) - 1);
	if (ret > 0) {
		read_buf[ret] = '\0';
		LOG_INF("FS read back: %s", read_buf);
	} else {
		LOG_ERR("FS read failed: %d", ret);
	}

	ret = plat_fs_file_size(test_path);
	LOG_INF("FS file size: %d bytes", ret);

	(void)plat_fs_delete_file(test_path);
}

#else
static void demo_filesystem(void)
{
	LOG_WRN("LittleFS not enabled on this platform");
}
#endif

/* ---- 网络/MQTT 演示 -------------------------------------------------------- */

#if defined(CONFIG_PLAT_NET) && defined(CONFIG_WIFI)

static void mqtt_rx_handler(const char *topic, uint16_t topic_len,
			    const uint8_t *payload, uint16_t payload_len,
			    void *user_data)
{
	ARG_UNUSED(user_data);

	LOG_INF("MQTT RX: topic=%.*s payload=%.*s",
		topic_len, topic, payload_len, payload);
}

static void demo_network(void)
{
	int ret;

	ret = plat_net_init();
	if (ret != 0) {
		LOG_WRN("Network init failed: %d", ret);
		return;
	}

	/* 实际使用时替换为真实 WiFi 凭据和 broker 地址 */
	ret = plat_net_connect("YOUR_SSID", "YOUR_PASSWORD");
	if (ret != 0) {
		LOG_WRN("Network connect failed: %d (expected in demo)", ret);
		return;
	}

	ret = plat_net_mqtt_connect("192.168.1.100", 1883, "zephyr-client",
				    mqtt_rx_handler, NULL);
	if (ret != 0) {
		LOG_WRN("MQTT connect failed: %d", ret);
		return;
	}

	(void)plat_net_mqtt_subscribe("zephyr/demo/rx", 0);
	(void)plat_net_mqtt_publish("zephyr/demo/tx",
				    (const uint8_t *)"hello", 5, 0);

	LOG_INF("MQTT demo running...");
}

#else
static void demo_network(void)
{
	LOG_WRN("Network not enabled on this platform");
}
#endif

/* ---- 设备电源管理演示 ------------------------------------------------------ */

#if defined(CONFIG_PLAT_PM)

static void demo_pm_runtime(void)
{
	int ret;
	int count = 0;

	/* 遍历设备，对支持 PM 的设备执行 get/put 演示 */
	STRUCT_SECTION_FOREACH(device, dev) {
		if (plat_pm_is_enabled(dev)) {
			ret = plat_pm_get(dev);
			if (ret == 0) {
				(void)plat_pm_put(dev);
				count++;
			}
		}
	}

	LOG_INF("PM runtime demo: %d devices support runtime PM", count);
}

#else
static void demo_pm_runtime(void)
{
	LOG_WRN("PM runtime not enabled");
}
#endif

/* ---- 入口 ------------------------------------------------------------------ */

void app_services_demo_start(void)
{
	LOG_INF("=== System services demo ===");

	demo_filesystem();
	demo_network();
	demo_pm_runtime();

	LOG_INF("=== System services demo done ===");
}
