/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（SPI）
 *
 * SPI 平台统一封装：标准传输、CS 控制、DMA 预留。
 * 业务代码只依赖 plat_spi_* 接口，不直接调用厂商 HAL。
 */
#ifndef PLATFORM_PLAT_SPI_H_
#define PLATFORM_PLAT_SPI_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>

/** SPI 配置参数 */
struct plat_spi_config {
	uint32_t frequency;	/**< 时钟频率（Hz） */
	uint8_t mode;		/**< SPI 模式：0/1/2/3（CPOL/CPHA） */
	uint8_t bits;		/**< 数据位宽，通常 8 */
	bool cs_active_high;	/**< CS 极性，默认低有效 */
};

/**
 * 初始化 SPI 主机
 * @param dev  SPI 设备指针（DEVICE_DT_GET 获取）
 * @param cfg  配置参数，NULL 使用默认：1MHz, Mode 0, 8-bit
 * @return 0 成功，<0 错误码
 */
int plat_spi_init(const struct device *dev, const struct plat_spi_config *cfg);

/**
 * SPI 全双工传输
 * @param dev      SPI 设备
 * @param tx_buf   发送缓冲区，NULL 表示只收
 * @param rx_buf   接收缓冲区，NULL 表示只发
 * @param len      数据长度
 * @param cs_gpio  CS GPIO 描述符，NULL 使用控制器默认 CS
 * @return 0 成功，<0 错误码
 */
int plat_spi_transceive(const struct device *dev,
			const uint8_t *tx_buf, uint8_t *rx_buf,
			size_t len, const struct gpio_dt_spec *cs_gpio);

/**
 * SPI 只写
 */
static inline int plat_spi_write(const struct device *dev,
				 const uint8_t *tx_buf, size_t len,
				 const struct gpio_dt_spec *cs_gpio)
{
	return plat_spi_transceive(dev, tx_buf, NULL, len, cs_gpio);
}

/**
 * SPI 只读
 */
static inline int plat_spi_read(const struct device *dev,
				uint8_t *rx_buf, size_t len,
				const struct gpio_dt_spec *cs_gpio)
{
	return plat_spi_transceive(dev, NULL, rx_buf, len, cs_gpio);
}

/**
 * 写寄存器（先写地址，再写数据）
 */
int plat_spi_write_reg(const struct device *dev, uint8_t reg_addr,
		       const uint8_t *data, size_t len,
		       const struct gpio_dt_spec *cs_gpio);

/**
 * 读寄存器（先写地址，再读数据）
 */
int plat_spi_read_reg(const struct device *dev, uint8_t reg_addr,
		      uint8_t *data, size_t len,
		      const struct gpio_dt_spec *cs_gpio);

#endif /* PLATFORM_PLAT_SPI_H_ */
