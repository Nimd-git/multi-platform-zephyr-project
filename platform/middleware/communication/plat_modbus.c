/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file plat_modbus.c
 * @brief Modbus RTU 主站封装实现（基于 Zephyr MODBUS 子系统）
 */

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/modbus/modbus.h>

#include <platform/plat_modbus.h>

LOG_MODULE_REGISTER(plat_modbus);

static bool client_ready;
static int mb_iface = -1;

static enum uart_config_parity parity_map(uint8_t p)
{
	switch (p) {
	case PLAT_MODBUS_PARITY_EVEN:
		return UART_CFG_PARITY_EVEN;
	case PLAT_MODBUS_PARITY_ODD:
		return UART_CFG_PARITY_ODD;
	default:
		return UART_CFG_PARITY_NONE;
	}
}

int plat_modbus_client_init(const struct plat_modbus_config *cfg)
{
	struct modbus_iface_param param = { 0 };
	int ret;

	if (client_ready) {
		return -EALREADY;
	}
	if (cfg == NULL || cfg->iface_name == NULL) {
		return -EINVAL;
	}

	mb_iface = modbus_iface_get_by_name(cfg->iface_name);
	if (mb_iface < 0) {
		LOG_ERR("modbus iface '%s' not found (check DT modbus-serial node)",
			cfg->iface_name);
		return -ENODEV;
	}

	param.mode = MODBUS_MODE_RTU;
	/* rx_timeout 单位为毫秒（uint32_t），非 k_timeout_t */
	param.rx_timeout = cfg->rx_timeout_ms ? cfg->rx_timeout_ms : 100;
	param.serial.baud = cfg->baudrate ? cfg->baudrate : 115200;
	param.serial.parity = parity_map(cfg->parity);
	param.serial.stop_bits =
		(cfg->stop_bits == 2) ? UART_CFG_STOP_BITS_2 : UART_CFG_STOP_BITS_1;

	ret = modbus_init_client(mb_iface, param);
	if (ret != 0) {
		LOG_ERR("modbus_init_client failed: %d", ret);
		mb_iface = -1;
		return ret;
	}

	client_ready = true;
	LOG_INF("modbus client ready on '%s' @%u baud", cfg->iface_name, param.serial.baud);
	return 0;
}

int plat_modbus_client_deinit(void)
{
	int ret;

	if (!client_ready || mb_iface < 0) {
		return -EALREADY;
	}

	ret = modbus_disable((uint8_t)mb_iface);
	client_ready = false;
	mb_iface = -1;
	return ret;
}

int plat_modbus_read_holding(uint8_t unit_id, uint16_t start_addr, uint16_t *regs, uint16_t num)
{
	if (!client_ready) {
		return -EACCES;
	}
	return modbus_read_holding_regs(mb_iface, unit_id, start_addr, regs, num);
}

int plat_modbus_read_input(uint8_t unit_id, uint16_t start_addr, uint16_t *regs, uint16_t num)
{
	if (!client_ready) {
		return -EACCES;
	}
	return modbus_read_input_regs(mb_iface, unit_id, start_addr, regs, num);
}

int plat_modbus_write_register(uint8_t unit_id, uint16_t addr, uint16_t value)
{
	if (!client_ready) {
		return -EACCES;
	}
	return modbus_write_holding_reg(mb_iface, unit_id, addr, value);
}

int plat_modbus_write_registers(uint8_t unit_id, uint16_t start_addr, const uint16_t *regs,
				uint16_t num)
{
	if (!client_ready) {
		return -EACCES;
	}
	return modbus_write_holding_regs(mb_iface, unit_id, start_addr, (uint16_t *)regs, num);
}

int plat_modbus_write_coil(uint8_t unit_id, uint16_t addr, bool value)
{
	if (!client_ready) {
		return -EACCES;
	}
	return modbus_write_coil(mb_iface, unit_id, addr, value);
}

#define PLAT_MODBUS_COIL_BUF_MAX 128u /* 位图缓冲上限：128 字节 = 1024 个线圈 */

int plat_modbus_write_coils(uint8_t unit_id, uint16_t start_addr, const bool *values, uint16_t num)
{
	uint8_t bitmap[PLAT_MODBUS_COIL_BUF_MAX];
	uint16_t n_bytes;

	if (!client_ready) {
		return -EACCES;
	}
	if (num == 0 || num > PLAT_MODBUS_COIL_BUF_MAX * 8) {
		return -EINVAL;
	}

	/* Zephyr modbus_write_coils 要求 MSB-first 位图：(num-1)/8+1 字节 */
	n_bytes = (uint16_t)(((num - 1) / 8) + 1);
	memset(bitmap, 0, n_bytes);
	for (uint16_t i = 0; i < num; i++) {
		if (values[i]) {
			bitmap[i / 8] |= (uint8_t)(1U << (7U - (i % 8)));
		}
	}
	return modbus_write_coils(mb_iface, unit_id, start_addr, bitmap, num);
}
