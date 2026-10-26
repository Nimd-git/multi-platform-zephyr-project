/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（I2C）
 *
 * I2C 平台统一封装：标准读写、设备扫描、寄存器访问。
 * 业务代码只依赖 plat_i2c_* 接口，不直接调用厂商 HAL。
 */
#ifndef PLATFORM_PLAT_I2C_H_
#define PLATFORM_PLAT_I2C_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>

/** I2C 配置参数 */
struct plat_i2c_config {
	uint32_t bitrate;	/**< 时钟频率：100K/400K/1M */
};

/**
 * 初始化 I2C 主机
 * @param dev  I2C 设备指针（DEVICE_DT_GET 获取）
 * @param cfg  配置参数，NULL 使用默认 400KHz
 * @return 0 成功，<0 错误码
 */
int plat_i2c_init(const struct device *dev, const struct plat_i2c_config *cfg);

/**
 * I2C 写数据
 * @param dev   I2C 设备
 * @param addr  从机地址（7-bit）
 * @param data  发送数据
 * @param len   数据长度
 * @return 0 成功，<0 错误码
 */
int plat_i2c_write(const struct device *dev, uint16_t addr,
		   const uint8_t *data, size_t len);

/**
 * I2C 读数据
 * @param dev   I2C 设备
 * @param addr  从机地址（7-bit）
 * @param data  接收缓冲区
 * @param len   读取长度
 * @return 0 成功，<0 错误码
 */
int plat_i2c_read(const struct device *dev, uint16_t addr,
		  uint8_t *data, size_t len);

/**
 * I2C 写-读复合传输（先写寄存器地址，再读数据）
 */
int plat_i2c_write_read(const struct device *dev, uint16_t addr,
			const uint8_t *tx_data, size_t tx_len,
			uint8_t *rx_data, size_t rx_len);

/**
 * 写从机寄存器（单字节地址）
 */
int plat_i2c_write_reg(const struct device *dev, uint16_t addr,
		       uint8_t reg_addr, const uint8_t *data, size_t len);

/**
 * 读从机寄存器（单字节地址）
 */
int plat_i2c_read_reg(const struct device *dev, uint16_t addr,
		      uint8_t reg_addr, uint8_t *data, size_t len);

/**
 * 扫描总线上的 I2C 设备
 * @param dev      I2C 设备
 * @param addr_buf 存储扫描到的地址数组
 * @param max_cnt  数组容量
 * @return 实际发现的设备数，<0 错误码
 */
int plat_i2c_scan(const struct device *dev, uint8_t *addr_buf, size_t max_cnt);

#endif /* PLATFORM_PLAT_I2C_H_ */
