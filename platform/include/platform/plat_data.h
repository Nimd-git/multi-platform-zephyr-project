/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file plat_data.h
 * @brief 数据中间件：JSON / CBOR 序列化封装
 *
 * JSON：基于 Zephyr json 库（CONFIG_JSON_LIBRARY），描述符（描述符数组）方式
 *       对结构体做编解码，详见 zephyr/data/json.h。
 * CBOR：基于 zcbor（CONFIG_ZCBOR），提供流式编码封装（map/list/标量）。
 */

#ifndef PLATFORM_PLAT_DATA_H_
#define PLATFORM_PLAT_DATA_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <zephyr/data/json.h>
#include <zcbor_encode.h>

/* ============================ JSON ============================ */

/**
 * @brief JSON 编码（描述符方式）
 *
 * @param descr json_obj_descr 数组
 * @param descr_len 数组元素个数
 * @param val 待编码结构体指针
 * @param buf 输出缓冲
 * @param buf_len 输出缓冲大小
 * @retval 0 成功；-EINVAL 参数非法；-ENOMEM 缓冲不足
 */
int plat_data_json_encode(const struct json_obj_descr *descr, size_t descr_len, const void *val,
			  char *buf, size_t buf_len);

/**
 * @brief JSON 解码（描述符方式）
 */
int plat_data_json_decode(const struct json_obj_descr *descr, size_t descr_len, const char *buf,
			  size_t buf_len, void *val);

/* ============================ CBOR ============================ */

/** CBOR 编码器上下文（内部含 zcbor 状态数组，勿直接访问 payload） */
struct plat_data_cbor {
	zcbor_state_t states[2];
	uint8_t *buf_start;
};

/**
 * @brief 初始化 CBOR 编码器
 *
 * @param c 编码器上下文
 * @param buf 输出缓冲（用户持有）
 * @param buf_size 缓冲大小
 */
int plat_data_cbor_encoder_init(struct plat_data_cbor *c, uint8_t *buf, size_t buf_size);

/** @brief 打开 map（max_entries 为条目上限） */
int plat_data_cbor_map_open(struct plat_data_cbor *c, size_t max_entries);

/** @brief 关闭 map */
int plat_data_cbor_map_close(struct plat_data_cbor *c);

/** @brief 打开 list（max_count 为元素上限） */
int plat_data_cbor_list_open(struct plat_data_cbor *c, size_t max_count);

/** @brief 关闭 list */
int plat_data_cbor_list_close(struct plat_data_cbor *c);

/** @brief 写入字符串键（写 map 时先 key 后 value） */
int plat_data_cbor_key_str(struct plat_data_cbor *c, const char *key);

/** @brief 写入 int32 / uint32 / bool / 字符串 / 字节串 */
int plat_data_cbor_put_i32(struct plat_data_cbor *c, int32_t v);
int plat_data_cbor_put_u32(struct plat_data_cbor *c, uint32_t v);
int plat_data_cbor_put_bool(struct plat_data_cbor *c, bool v);
int plat_data_cbor_put_str(struct plat_data_cbor *c, const char *str);
int plat_data_cbor_put_bytes(struct plat_data_cbor *c, const uint8_t *data, size_t len);

/**
 * @brief 结束编码并取回长度
 *
 * @param c 编码器上下文
 * @param encoded_len 输出实际编码字节数
 */
int plat_data_cbor_finish(struct plat_data_cbor *c, size_t *encoded_len);

#endif /* PLATFORM_PLAT_DATA_H_ */
