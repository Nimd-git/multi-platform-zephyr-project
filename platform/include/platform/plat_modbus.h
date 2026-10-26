/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file plat_modbus.h
 * @brief 通信中间件：Modbus RTU 主站（client）封装
 *
 * 基于 Zephyr MODBUS 子系统（RTU over UART）。接口通过设备树
 * zephyr,modbus-serial 节点绑定（节点挂在 UART 控制器下），
 * iface_name 即该节点名（如 "modbus0"）。
 */

#ifndef PLATFORM_PLAT_MODBUS_H_
#define PLATFORM_PLAT_MODBUS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* 串口校验位（映射 Zephyr uart_config_parity） */
#define PLAT_MODBUS_PARITY_NONE 0
#define PLAT_MODBUS_PARITY_EVEN 1
#define PLAT_MODBUS_PARITY_ODD 2

struct plat_modbus_config {
	/** zephyr,modbus-serial 设备树节点名，如 "modbus0" */
	const char *iface_name;
	/** 波特率（RTU 常用 9600/115200） */
	uint32_t baudrate;
	/** 校验位 PLAT_MODBUS_PARITY_* */
	uint8_t parity;
	/** 停止位 1 或 2 */
	uint8_t stop_bits;
	/** 主站等待从站响应超时（毫秒） */
	uint32_t rx_timeout_ms;
};

/**
 * @brief 初始化并使能 Modbus RTU 主站
 *
 * @retval 0 成功；-EINVAL 参数非法；-ENODEV 找不到接口节点；其他为底层错误码
 */
int plat_modbus_client_init(const struct plat_modbus_config *cfg);

/**
 * @brief 关闭 Modbus 主站接口
 */
int plat_modbus_client_deinit(void);

/**
 * @brief 读保持寄存器（0x03）
 *
 * @param unit_id 从站地址
 * @param start_addr 起始寄存器地址
 * @param regs 输出缓冲
 * @param num 寄存器数量
 */
int plat_modbus_read_holding(uint8_t unit_id, uint16_t start_addr, uint16_t *regs, uint16_t num);

/**
 * @brief 读输入寄存器（0x04）
 */
int plat_modbus_read_input(uint8_t unit_id, uint16_t start_addr, uint16_t *regs, uint16_t num);

/**
 * @brief 写单个保持寄存器（0x06）
 */
int plat_modbus_write_register(uint8_t unit_id, uint16_t addr, uint16_t value);

/**
 * @brief 写多个保持寄存器（0x10）
 */
int plat_modbus_write_registers(uint8_t unit_id, uint16_t start_addr, const uint16_t *regs,
				uint16_t num);

/**
 * @brief 写单个线圈（0x05）
 */
int plat_modbus_write_coil(uint8_t unit_id, uint16_t addr, bool value);

/**
 * @brief 写多个线圈（0x0F）
 */
int plat_modbus_write_coils(uint8_t unit_id, uint16_t start_addr, const bool *values, uint16_t num);

#endif /* PLATFORM_PLAT_MODBUS_H_ */
