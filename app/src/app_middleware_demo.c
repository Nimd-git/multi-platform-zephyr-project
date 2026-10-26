// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：应用开发层 -> 中间件演示
 *
 *   - 存储：plat_kv 键值（u32 + blob + delete）
 *   - 数据：JSON 描述符编解码、CBOR 流式编码
 *   - 通信：Modbus RTU 主站（需从站在线，演示仅记录结果）
 *   - UWB ：SR040 init/设备信息/会话（需板级接线，未接线时返回 -ENODEV）
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

#include <platform/plat_kv.h>
#include <platform/plat_data.h>
#if defined(CONFIG_PLAT_MW_COMMUNICATION)
#include <platform/plat_modbus.h>
#endif
#if defined(CONFIG_PLAT_MW_UWB)
#include <platform/plat_uwb.h>
#endif

#include "app_middleware_demo.h"

LOG_MODULE_REGISTER(middleware_demo);

/* ---- 存储中间件（KV） ------------------------------------------------------ */

static void demo_kv(void)
{
	uint32_t counter = 0;
	uint8_t blob[16] = "plat-blob-demo";
	uint8_t readback[PLAT_KV_MAX_VAL_LEN];
	size_t blob_len = 0;
	int ret;

	ret = plat_kv_get_u32("plat/mw/demo/counter", &counter);
	if (ret == 0) {
		counter++;
	} else {
		counter = 1;
	}
	ret = plat_kv_set_u32("plat/mw/demo/counter", counter);
	LOG_INF("KV counter set=%d value=%u", ret, counter);

	ret = plat_kv_set_blob("plat/mw/demo/blob", blob, 14);
	if (ret == 0) {
		ret = plat_kv_get_blob("plat/mw/demo/blob", readback, sizeof(readback), &blob_len);
	}
	if (ret == 0) {
		readback[blob_len] = '\0';
		LOG_INF("KV blob readback (%u bytes): %s", blob_len, readback);
	} else {
		LOG_ERR("KV blob failed: %d", ret);
	}

	ret = plat_kv_delete("plat/mw/demo/blob");
	LOG_INF("KV blob delete=%d, get again=%d", ret,
		plat_kv_get_blob("plat/mw/demo/blob", readback, sizeof(readback), &blob_len));
}

/* ---- 数据中间件（JSON / CBOR） --------------------------------------------- */

struct demo_point {
	uint16_t seq;
	int32_t temp;
	bool valid;
};

static const struct json_obj_descr demo_point_descr[] = {
	JSON_OBJ_DESCR_PRIM(struct demo_point, seq, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct demo_point, temp, JSON_TOK_NUMBER),
	JSON_OBJ_DESCR_PRIM(struct demo_point, valid, JSON_TOK_TRUE),
};

static void demo_json(void)
{
	struct demo_point point = { .seq = 42, .temp = -1250, .valid = true };
	struct demo_point parsed = { 0 };
	char buf[128];
	int ret;

	ret = plat_data_json_encode(demo_point_descr, ARRAY_SIZE(demo_point_descr), &point, buf,
				    sizeof(buf));
	if (ret != 0) {
		LOG_ERR("JSON encode failed: %d", ret);
		return;
	}
	LOG_INF("JSON: %s", buf);

	ret = plat_data_json_decode(demo_point_descr, ARRAY_SIZE(demo_point_descr), buf,
				    strlen(buf), &parsed);
	LOG_INF("JSON parse=%d seq=%u temp=%d valid=%d", ret, parsed.seq, parsed.temp,
		parsed.valid);
}

static void demo_cbor(void)
{
	struct plat_data_cbor cbor;
	uint8_t buf[128];
	size_t len = 0;
	int ret;

	ret = plat_data_cbor_encoder_init(&cbor, buf, sizeof(buf));
	if (ret == 0) {
		ret |= plat_data_cbor_map_open(&cbor, 3);
		ret |= plat_data_cbor_key_str(&cbor, "seq");
		ret |= plat_data_cbor_put_u32(&cbor, 42);
		ret |= plat_data_cbor_key_str(&cbor, "temp");
		ret |= plat_data_cbor_put_i32(&cbor, -1250);
		ret |= plat_data_cbor_key_str(&cbor, "valid");
		ret |= plat_data_cbor_put_bool(&cbor, true);
		ret |= plat_data_cbor_map_close(&cbor);
		ret |= plat_data_cbor_finish(&cbor, &len);
	}
	if (ret == 0) {
		LOG_INF("CBOR encoded %u bytes: %02x %02x %02x %02x ...", len, buf[0], buf[1],
			buf[2], buf[3]);
	} else {
		LOG_ERR("CBOR encode failed: %d", ret);
	}
}

/* ---- 通信中间件（Modbus RTU 主站） ----------------------------------------- */

#if defined(CONFIG_PLAT_MW_COMMUNICATION)

static void demo_modbus(void)
{
	const struct plat_modbus_config cfg = {
		.iface_name = "modbus0",
		.baudrate = 115200,
		.parity = PLAT_MODBUS_PARITY_NONE,
		.stop_bits = 1,
		.rx_timeout_ms = 200,
	};
	uint16_t regs[8] = { 0 };
	int ret;

	ret = plat_modbus_client_init(&cfg);
	if (ret != 0) {
		LOG_ERR("Modbus client init failed: %d", ret);
		return;
	}

	/* 从站地址 1 的保持寄存器 0x0000 起 4 个；无从站在线时超时报错，属预期 */
	ret = plat_modbus_read_holding(1, 0x0000, regs, 4);
	LOG_INF("Modbus read_holding ret=%d regs=%04x %04x %04x %04x", ret, regs[0], regs[1],
		regs[2], regs[3]);
}

#else
static void demo_modbus(void)
{
	LOG_WRN("Modbus middleware not enabled on this platform");
}
#endif

/* ---- UWB 中间件（SR040） --------------------------------------------------- */

#if defined(CONFIG_PLAT_MW_UWB)

static void demo_uwb(void)
{
#if DT_NODE_EXISTS(DT_NODELABEL(nxp_uwb_device))
	struct plat_uwb_dev_info info;
	uint32_t handle = 0;
	int ret;

	ret = plat_uwb_init();
	if (ret != 0) {
		LOG_ERR("UWB init failed: %d", ret);
		return;
	}

	ret = plat_uwb_get_device_info(&info);
	if (ret == 0) {
		LOG_INF("UWB dev: name=%s sn=%s fw=%u.%u.%u", info.dev_name, info.serial_no,
			info.fw_major, info.fw_minor, info.fw_patch);
	} else {
		LOG_WRN("UWB get_device_info failed: %d", ret);
	}

	ret = plat_uwb_session_start(1, &handle);
	if (ret == 0) {
		LOG_INF("UWB session running, handle=0x%08x", handle);
	} else {
		LOG_WRN("UWB session start failed: %d", ret);
	}
#else
	LOG_WRN("SR040 not wired on this board (no nxp_uwb_device DT node): %d", -ENODEV);
#endif
}

#else
static void demo_uwb(void)
{
	LOG_WRN("UWB middleware not enabled on this platform");
}
#endif

void app_middleware_demo_start(void)
{
	demo_kv();
	demo_json();
	demo_cbor();
	demo_modbus();
	demo_uwb();
}
