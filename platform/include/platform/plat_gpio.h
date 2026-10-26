/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（GPIO/UART/SPI/I2C）
 *
 * GPIO 平台统一封装：输出、输入、中断、LED 专用操作。
 * 业务代码只依赖 plat_gpio_* 接口，不直接调用厂商 HAL。
 */
#ifndef PLATFORM_PLAT_GPIO_H_
#define PLATFORM_PLAT_GPIO_H_

#include <stdbool.h>
#include <zephyr/drivers/gpio.h>

/** GPIO 工作模式 */
enum plat_gpio_mode {
	PLAT_GPIO_MODE_OUTPUT,	/**< 推挽输出 */
	PLAT_GPIO_MODE_INPUT,	/**< 输入（无上下拉） */
	PLAT_GPIO_MODE_INPUT_PULLUP,	/**< 输入上拉 */
	PLAT_GPIO_MODE_INPUT_PULLDOWN,	/**< 输入下拉 */
	PLAT_GPIO_MODE_INT_RISING,	/**< 上升沿中断 */
	PLAT_GPIO_MODE_INT_FALLING,	/**< 下降沿中断 */
	PLAT_GPIO_MODE_INT_BOTH,	/**< 双边沿中断 */
};

/** GPIO 中断回调函数类型 */
typedef void (*plat_gpio_isr_t)(const struct device *port, uint32_t pin,
				void *user_data);

/**
 * 初始化 GPIO（通用）
 * @param spec  设备树 GPIO 描述符
 * @param mode  工作模式
 * @return 0 成功，<0 错误码
 */
int plat_gpio_init(const struct gpio_dt_spec *spec, enum plat_gpio_mode mode);

/**
 * 将 gpio_dt_spec 初始化为推挽输出（初始低电平）
 * @note 兼容旧接口，等价于 plat_gpio_init(spec, PLAT_GPIO_MODE_OUTPUT)
 */
int plat_gpio_output_init(const struct gpio_dt_spec *spec);

/** LED 开/关（spec 通常由设备树 GPIO_DT_SPEC_GET 得到） */
int plat_led_set(const struct gpio_dt_spec *spec, bool on);

/** 翻转 LED，返回翻转后的电平：1=高，0=低，<0=错误 */
int plat_led_toggle(const struct gpio_dt_spec *spec);

/**
 * 读取 GPIO 输入电平
 * @return 1=高电平，0=低电平，<0=错误码
 */
int plat_gpio_input_get(const struct gpio_dt_spec *spec);

/**
 * 注册 GPIO 中断回调
 * @param spec      设备树 GPIO 描述符
 * @param callback  中断回调（在 ISR 上下文执行）
 * @param user_data 用户数据指针，透传给回调
 * @return 0 成功，<0 错误码
 * @note 需先调用 plat_gpio_init 配置为中断模式
 */
int plat_gpio_interrupt_register(const struct gpio_dt_spec *spec,
				 plat_gpio_isr_t callback, void *user_data);

/**
 * 注销 GPIO 中断
 */
int plat_gpio_interrupt_unregister(const struct gpio_dt_spec *spec);

#endif /* PLATFORM_PLAT_GPIO_H_ */
