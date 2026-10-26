// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（GPIO）
 * 对上游 GPIO API 的平台统一封装；后续 UART/SPI/I2C、ADC/PWM 同模式扩展。
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include <platform/plat_gpio.h>

struct gpio_isr_ctx {
	plat_gpio_isr_t cb;
	void *user_data;
	const struct gpio_dt_spec *spec;
	struct gpio_callback gpio_cb;
};

static struct gpio_isr_ctx isr_ctxs[8];
static uint8_t isr_ctx_used;

static void gpio_isr_bridge(const struct device *port,
			    struct gpio_callback *cb, uint32_t pins)
{
	struct gpio_isr_ctx *ctx = CONTAINER_OF(cb, struct gpio_isr_ctx, gpio_cb);

	if (ctx->cb != NULL) {
		ctx->cb(port, pins, ctx->user_data);
	}
}

int plat_gpio_init(const struct gpio_dt_spec *spec, enum plat_gpio_mode mode)
{
	gpio_flags_t flags;

	if ((spec == NULL) || (spec->port == NULL)) {
		return -EINVAL;
	}
	if (!gpio_is_ready_dt(spec)) {
		return -ENODEV;
	}

	switch (mode) {
	case PLAT_GPIO_MODE_OUTPUT:
		flags = GPIO_OUTPUT_INACTIVE;
		break;
	case PLAT_GPIO_MODE_INPUT:
		flags = GPIO_INPUT;
		break;
	case PLAT_GPIO_MODE_INPUT_PULLUP:
		flags = GPIO_INPUT | GPIO_PULL_UP;
		break;
	case PLAT_GPIO_MODE_INPUT_PULLDOWN:
		flags = GPIO_INPUT | GPIO_PULL_DOWN;
		break;
	case PLAT_GPIO_MODE_INT_RISING:
		flags = GPIO_INPUT | GPIO_INT_EDGE_RISING;
		break;
	case PLAT_GPIO_MODE_INT_FALLING:
		flags = GPIO_INPUT | GPIO_INT_EDGE_FALLING;
		break;
	case PLAT_GPIO_MODE_INT_BOTH:
		flags = GPIO_INPUT | GPIO_INT_EDGE_BOTH;
		break;
	default:
		return -EINVAL;
	}

	return gpio_pin_configure_dt(spec, flags);
}

int plat_gpio_output_init(const struct gpio_dt_spec *spec)
{
	return plat_gpio_init(spec, PLAT_GPIO_MODE_OUTPUT);
}

int plat_led_set(const struct gpio_dt_spec *spec, bool on)
{
	if ((spec == NULL) || (spec->port == NULL)) {
		return -EINVAL;
	}
	return gpio_pin_set_dt(spec, on ? 1 : 0);
}

int plat_led_toggle(const struct gpio_dt_spec *spec)
{
	int ret;

	if ((spec == NULL) || (spec->port == NULL)) {
		return -EINVAL;
	}

	ret = gpio_pin_toggle_dt(spec);
	if (ret < 0) {
		return ret;
	}

	/* 上游 gpio_pin_toggle_dt() 成功时返回状态码 0，而非翻转后的电平；
	 * 回读引脚实际电平以满足头文件契约（1=高，0=低，<0=错误）。 */
	return gpio_pin_get_dt(spec);
}

int plat_gpio_input_get(const struct gpio_dt_spec *spec)
{
	if ((spec == NULL) || (spec->port == NULL)) {
		return -EINVAL;
	}
	return gpio_pin_get_dt(spec);
}

int plat_gpio_interrupt_register(const struct gpio_dt_spec *spec,
				 plat_gpio_isr_t callback, void *user_data)
{
	uint8_t i;
	struct gpio_isr_ctx *ctx = NULL;
	int ret;

	if ((spec == NULL) || (spec->port == NULL) || (callback == NULL)) {
		return -EINVAL;
	}

	/* 查找空闲槽位 */
	for (i = 0; i < ARRAY_SIZE(isr_ctxs); i++) {
		if (!(isr_ctx_used & BIT(i))) {
			ctx = &isr_ctxs[i];
			isr_ctx_used |= BIT(i);
			break;
		}
	}
	if (ctx == NULL) {
		return -ENOMEM;
	}

	ctx->cb = callback;
	ctx->user_data = user_data;
	ctx->spec = spec;

	gpio_init_callback(&ctx->gpio_cb, gpio_isr_bridge, BIT(spec->pin));
	ret = gpio_add_callback(spec->port, &ctx->gpio_cb);
	if (ret != 0) {
		isr_ctx_used &= ~BIT(i);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		(void)gpio_remove_callback(spec->port, &ctx->gpio_cb);
		isr_ctx_used &= ~BIT(i);
		return ret;
	}

	return 0;
}

int plat_gpio_interrupt_unregister(const struct gpio_dt_spec *spec)
{
	uint8_t i;

	if ((spec == NULL) || (spec->port == NULL)) {
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(isr_ctxs); i++) {
		if ((isr_ctx_used & BIT(i)) && (isr_ctxs[i].spec == spec)) {
			(void)gpio_pin_interrupt_configure_dt(spec, GPIO_INT_DISABLE);
			(void)gpio_remove_callback(spec->port, &isr_ctxs[i].gpio_cb);
			isr_ctx_used &= ~BIT(i);
			return 0;
		}
	}

	return -ENOENT;
}
