// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（SPI）
 * 对上游 SPI API 的平台统一封装。
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <errno.h>

#include <platform/plat_spi.h>

/* 默认配置：1MHz, Mode 0, 8-bit */
static const struct plat_spi_config default_cfg = {
	.frequency = 1000000,
	.mode = 0,
	.bits = 8,
	.cs_active_high = false,
};

struct spi_ctx {
	struct spi_config cfg;
	struct spi_cs_control cs_ctrl;
	bool cs_is_gpio;
};

static struct spi_ctx spi_ctxs[4];
static uint8_t spi_ctx_used;

static int spi_ctx_alloc(const struct device *dev,
			 const struct plat_spi_config *cfg,
			 const struct gpio_dt_spec *cs_gpio,
			 struct spi_ctx **out)
{
	uint8_t i;
	struct spi_ctx *ctx = NULL;

	for (i = 0; i < ARRAY_SIZE(spi_ctxs); i++) {
		if (!(spi_ctx_used & BIT(i))) {
			ctx = &spi_ctxs[i];
			spi_ctx_used |= BIT(i);
			break;
		}
	}
	if (ctx == NULL) {
		return -ENOMEM;
	}

	if (cfg == NULL) {
		cfg = &default_cfg;
	}

	ctx->cfg.frequency = cfg->frequency;
	ctx->cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(cfg->bits);
	ctx->cfg.slave = 0;
	memset(&ctx->cfg.cs, 0, sizeof(ctx->cfg.cs));

	switch (cfg->mode) {
	case 0:
		ctx->cfg.operation |= SPI_MODE_CPOL | SPI_MODE_CPHA;
		break;
	case 1:
		ctx->cfg.operation |= SPI_MODE_CPHA;
		break;
	case 2:
		ctx->cfg.operation |= SPI_MODE_CPOL;
		break;
	case 3:
	default:
		break;
	}

	if (cs_gpio != NULL) {
		ctx->cs_ctrl.gpio.port = cs_gpio->port;
		ctx->cs_ctrl.gpio.pin = cs_gpio->pin;
		ctx->cs_ctrl.gpio.dt_flags = cs_gpio->dt_flags;
		ctx->cs_ctrl.delay = 0;
		ctx->cfg.cs = ctx->cs_ctrl;
		ctx->cs_is_gpio = true;
	} else {
		ctx->cs_is_gpio = false;
	}

	*out = ctx;
	return 0;
}

static void spi_ctx_free(struct spi_ctx *ctx)
{
	uint8_t i;

	for (i = 0; i < ARRAY_SIZE(spi_ctxs); i++) {
		if ((&spi_ctxs[i] == ctx) && (spi_ctx_used & BIT(i))) {
			spi_ctx_used &= ~BIT(i);
			return;
		}
	}
}

int plat_spi_init(const struct device *dev, const struct plat_spi_config *cfg)
{
	ARG_UNUSED(cfg);

	if (dev == NULL) {
		return -EINVAL;
	}
	if (!device_is_ready(dev)) {
		return -ENODEV;
	}

	return 0;
}

int plat_spi_transceive(const struct device *dev,
			const uint8_t *tx_buf, uint8_t *rx_buf,
			size_t len, const struct gpio_dt_spec *cs_gpio)
{
	struct spi_ctx *ctx;
	struct spi_buf tx = { .buf = (void *)tx_buf, .len = len };
	struct spi_buf rx = { .buf = rx_buf, .len = len };
	struct spi_buf_set tx_set = { .buffers = &tx, .count = tx_buf ? 1 : 0 };
	struct spi_buf_set rx_set = { .buffers = &rx, .count = rx_buf ? 1 : 0 };
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}
	if ((tx_buf == NULL) && (rx_buf == NULL)) {
		return -EINVAL;
	}

	ret = spi_ctx_alloc(dev, NULL, cs_gpio, &ctx);
	if (ret != 0) {
		return ret;
	}

	ret = spi_transceive(dev, &ctx->cfg, &tx_set, &rx_set);
	spi_ctx_free(ctx);
	return ret;
}

int plat_spi_write_reg(const struct device *dev, uint8_t reg_addr,
		       const uint8_t *data, size_t len,
		       const struct gpio_dt_spec *cs_gpio)
{
	struct spi_ctx *ctx;
	struct spi_buf tx[2] = {
		{ .buf = &reg_addr, .len = 1 },
		{ .buf = (void *)data, .len = len },
	};
	struct spi_buf_set tx_set = { .buffers = tx, .count = 2 };
	int ret;

	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	ret = spi_ctx_alloc(dev, NULL, cs_gpio, &ctx);
	if (ret != 0) {
		return ret;
	}

	ret = spi_write(dev, &ctx->cfg, &tx_set);
	spi_ctx_free(ctx);
	return ret;
}

int plat_spi_read_reg(const struct device *dev, uint8_t reg_addr,
		      uint8_t *data, size_t len,
		      const struct gpio_dt_spec *cs_gpio)
{
	struct spi_ctx *ctx;
	struct spi_buf tx = { .buf = &reg_addr, .len = 1 };
	struct spi_buf rx = { .buf = data, .len = len };
	struct spi_buf_set tx_set = { .buffers = &tx, .count = 1 };
	struct spi_buf_set rx_set = { .buffers = &rx, .count = 1 };
	int ret;

	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	ret = spi_ctx_alloc(dev, NULL, cs_gpio, &ctx);
	if (ret != 0) {
		return ret;
	}

	ret = spi_transceive(dev, &ctx->cfg, &tx_set, &rx_set);
	spi_ctx_free(ctx);
	return ret;
}
