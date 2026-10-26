// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（UART）
 * 对上游 UART API 的平台统一封装。
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <errno.h>

#include <platform/plat_uart.h>

/* 默认配置：115200-8-N-1 */
static const struct plat_uart_config default_cfg = {
	.baudrate = 115200,
	.data_bits = 8,
	.stop_bits = 1,
	.parity = 0,
	.flow_ctrl = false,
};

struct uart_rx_ctx {
	plat_uart_rx_cb_t cb;
	void *user_data;
};

static struct uart_rx_ctx rx_ctxs[4];
static uint8_t rx_ctx_used;

static void uart_rx_isr_bridge(const struct device *dev, void *user_data)
{
	struct uart_rx_ctx *ctx = user_data;
	uint8_t buf[32];
	int len;

	if (ctx->cb == NULL) {
		return;
	}

	while ((len = uart_fifo_read(dev, buf, sizeof(buf))) > 0) {
		ctx->cb(dev, buf, len, ctx->user_data);
	}
}

int plat_uart_init(const struct device *dev, const struct plat_uart_config *cfg)
{
	struct uart_config zcfg;
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

	zcfg.baudrate = cfg->baudrate;
	zcfg.data_bits = cfg->data_bits;
	zcfg.stop_bits = cfg->stop_bits;
	zcfg.parity = cfg->parity;
	zcfg.flow_ctrl = cfg->flow_ctrl;

	ret = uart_configure(dev, &zcfg);
	if (ret != 0) {
		return ret;
	}

	/* 默认关闭中断接收，由用户显式调用 plat_uart_rx_enable */
	(void)uart_irq_rx_disable(dev);

	return 0;
}

int plat_uart_poll_out(const struct device *dev, uint8_t byte)
{
	if (dev == NULL) {
		return -EINVAL;
	}

	uart_poll_out(dev, byte);
	return 0;
}

int plat_uart_poll_in(const struct device *dev, uint8_t *byte, int32_t timeout_ms)
{
	int64_t end_time;
	int ret;

	if ((dev == NULL) || (byte == NULL)) {
		return -EINVAL;
	}

	end_time = k_uptime_get() + timeout_ms;

	while (1) {
		ret = uart_poll_in(dev, byte);
		if (ret == 0) {
			return 0;
		}
		if ((timeout_ms >= 0) && (k_uptime_get() >= end_time)) {
			return -EAGAIN;
		}
		k_yield();
	}
}

int plat_uart_write(const struct device *dev, const uint8_t *data, size_t len)
{
	size_t i;

	if ((dev == NULL) || (data == NULL)) {
		return -EINVAL;
	}

	for (i = 0; i < len; i++) {
		uart_poll_out(dev, data[i]);
	}

	return (int)len;
}

int plat_uart_rx_enable(const struct device *dev, plat_uart_rx_cb_t cb,
			void *user_data)
{
	uint8_t i;
	struct uart_rx_ctx *ctx = NULL;

	if ((dev == NULL) || (cb == NULL)) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(rx_ctxs); i++) {
		if (!(rx_ctx_used & BIT(i))) {
			ctx = &rx_ctxs[i];
			rx_ctx_used |= BIT(i);
			break;
		}
	}
	if (ctx == NULL) {
		return -ENOMEM;
	}

	ctx->cb = cb;
	ctx->user_data = user_data;

	uart_irq_callback_user_data_set(dev, uart_rx_isr_bridge, ctx);
	uart_irq_rx_enable(dev);

	return 0;
}

int plat_uart_rx_disable(const struct device *dev)
{
	uint8_t i;

	if (dev == NULL) {
		return -EINVAL;
	}

	uart_irq_rx_disable(dev);

	for (i = 0; i < ARRAY_SIZE(rx_ctxs); i++) {
		if (rx_ctx_used & BIT(i)) {
			rx_ctx_used &= ~BIT(i);
			return 0;
		}
	}

	return -ENOENT;
}

int plat_uart_write_async(const struct device *dev, const uint8_t *data,
			  size_t len)
{
	/* 当前版本内部仍使用轮询发送，后续可切换为中断/DMA */
	return plat_uart_write(dev, data, len);
}
