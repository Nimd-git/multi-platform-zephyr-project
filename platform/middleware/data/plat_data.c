/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file plat_data.c
 * @brief 数据中间件实现：JSON（Zephyr json 库）+ CBOR（zcbor 流式编码）
 */

#include <errno.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <platform/plat_data.h>

LOG_MODULE_REGISTER(plat_data);

/* ============================ JSON ============================ */

int plat_data_json_encode(const struct json_obj_descr *descr, size_t descr_len, const void *val,
			  char *buf, size_t buf_len)
{
	ssize_t ret;

	if (descr == NULL || val == NULL || buf == NULL || buf_len == 0) {
		return -EINVAL;
	}

	ret = json_obj_encode_buf(descr, descr_len, val, buf, buf_len);
	if (ret < 0) {
		return (ret == -EINVAL) ? -EINVAL : -ENOMEM;
	}
	return 0;
}

int plat_data_json_decode(const struct json_obj_descr *descr, size_t descr_len, const char *buf,
			  size_t buf_len, void *val)
{
	if (descr == NULL || buf == NULL || buf_len == 0 || val == NULL) {
		return -EINVAL;
	}

	return json_obj_parse((char *)buf, buf_len, descr, descr_len, val);
}

/* ============================ CBOR ============================ */

int plat_data_cbor_encoder_init(struct plat_data_cbor *c, uint8_t *buf, size_t buf_size)
{
	if (c == NULL || buf == NULL || buf_size == 0) {
		return -EINVAL;
	}

	zcbor_new_state(c->states, ARRAY_SIZE(c->states), buf, buf_size, 0, NULL, 0);
	c->buf_start = buf;
	return 0;
}

int plat_data_cbor_map_open(struct plat_data_cbor *c, size_t max_entries)
{
	if (c == NULL || !zcbor_map_start_encode(c->states, max_entries)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_map_close(struct plat_data_cbor *c)
{
	if (c == NULL || !zcbor_map_end_encode(c->states, ZCBOR_MAX_ELEM_COUNT)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_list_open(struct plat_data_cbor *c, size_t max_count)
{
	if (c == NULL || !zcbor_list_start_encode(c->states, max_count)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_list_close(struct plat_data_cbor *c)
{
	if (c == NULL || !zcbor_list_end_encode(c->states, ZCBOR_MAX_ELEM_COUNT)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_key_str(struct plat_data_cbor *c, const char *key)
{
	if (c == NULL || key == NULL) {
		return -EINVAL;
	}
	if (!zcbor_tstr_put_lit(c->states, key)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_put_i32(struct plat_data_cbor *c, int32_t v)
{
	if (c == NULL || !zcbor_int32_put(c->states, v)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_put_u32(struct plat_data_cbor *c, uint32_t v)
{
	if (c == NULL || !zcbor_uint32_put(c->states, v)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_put_bool(struct plat_data_cbor *c, bool v)
{
	if (c == NULL || !zcbor_bool_put(c->states, v)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_put_str(struct plat_data_cbor *c, const char *str)
{
	if (c == NULL || str == NULL) {
		return -EINVAL;
	}
	if (!zcbor_tstr_put_lit(c->states, str)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_put_bytes(struct plat_data_cbor *c, const uint8_t *data, size_t len)
{
	struct zcbor_string str;

	if (c == NULL || (data == NULL && len != 0)) {
		return -EINVAL;
	}

	str.value = data;
	str.len = len;
	if (!zcbor_bstr_encode(c->states, &str)) {
		return -EINVAL;
	}
	return 0;
}

int plat_data_cbor_finish(struct plat_data_cbor *c, size_t *encoded_len)
{
	if (c == NULL) {
		return -EINVAL;
	}

	if (encoded_len != NULL) {
		*encoded_len = (size_t)(c->states[0].payload - c->buf_start);
	}
	return 0;
}
