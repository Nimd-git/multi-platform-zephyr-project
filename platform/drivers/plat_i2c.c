// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（I2C）
 * 对上游 I2C API 的平台统一封装。
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <errno.h>

#include <platform/plat_i2c.h>

/* 默认配置：400KHz */
static const struct plat_i2c_config default_cfg = {
	.bitrate = 400000,
};

int plat_i2c_init(const struct device *dev, const struct plat_i2c_config *cfg)
{
	uint32_t i2c_speed;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	if (cfg == NULL) {
		cfg = &default_cfg;
	}

	if (cfg->bitrate <= 100000) {
		i2c_speed = I2C_SPEED_STANDARD;
	} else if (cfg->bitrate <= 400000) {
		i2c_speed = I2C_SPEED_FAST;
	} else if (cfg->bitrate <= 1000000) {
		i2c_speed = I2C_SPEED_FAST_PLUS;
	} else {
		i2c_speed = I2C_SPEED_HIGH;
	}

	ret = i2c_configure(dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(i2c_speed));
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int plat_i2c_write(const struct device *dev, uint16_t addr,
		   const uint8_t *data, size_t len)
{
	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	return i2c_write(dev, data, len, addr);
}

int plat_i2c_read(const struct device *dev, uint16_t addr,
		  uint8_t *data, size_t len)
{
	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	return i2c_read(dev, data, len, addr);
}

int plat_i2c_write_read(const struct device *dev, uint16_t addr,
			const uint8_t *tx_data, size_t tx_len,
			uint8_t *rx_data, size_t rx_len)
{
	if ((dev == NULL) || (tx_data == NULL) || (rx_data == NULL)) {
		return -EINVAL;
	}

	return i2c_write_read(dev, addr, tx_data, tx_len, rx_data, rx_len);
}

int plat_i2c_write_reg(const struct device *dev, uint16_t addr,
		       uint8_t reg_addr, const uint8_t *data, size_t len)
{
	uint8_t buf[16];

	if ((dev == NULL) || (data == NULL) || (len > sizeof(buf) - 1)) {
		return -EINVAL;
	}

	buf[0] = reg_addr;
	memcpy(&buf[1], data, len);

	return i2c_write(dev, buf, len + 1, addr);
}

int plat_i2c_read_reg(const struct device *dev, uint16_t addr,
		      uint8_t reg_addr, uint8_t *data, size_t len)
{
	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	return i2c_write_read(dev, addr, &reg_addr, 1, data, len);
}

int plat_i2c_scan(const struct device *dev, uint8_t *addr_buf, size_t max_cnt)
{
	size_t found = 0;
	uint16_t addr;
	uint8_t dummy;
	int ret;

	if ((dev == NULL) || (addr_buf == NULL)) {
		return -EINVAL;
	}

	for (addr = 0x03; addr <= 0x77 && found < max_cnt; addr++) {
		ret = i2c_read(dev, &dummy, 1, addr);
		if (ret == 0) {
			addr_buf[found++] = (uint8_t)addr;
		}
	}

	return (int)found;
}
