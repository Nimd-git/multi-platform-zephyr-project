/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：硬件抽象层 -> 驱动 API 标准化（UART）
 *
 * UART 平台统一封装：轮询收发、中断收发、RS485 方向控制预留。
 * 业务代码只依赖 plat_uart_* 接口，不直接调用厂商 HAL。
 */
#ifndef PLATFORM_PLAT_UART_H_
#define PLATFORM_PLAT_UART_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>

/** UART 配置参数 */
struct plat_uart_config {
	uint32_t baudrate;	/**< 波特率 */
	uint8_t data_bits;	/**< 数据位：5/6/7/8 */
	uint8_t stop_bits;	/**< 停止位：1/2 */
	uint8_t parity;		/**< 校验：0=None, 1=Odd, 2=Even */
	bool flow_ctrl;		/**< 硬件流控 */
};

/** UART 接收回调（在 ISR 上下文执行） */
typedef void (*plat_uart_rx_cb_t)(const struct device *dev,
				  const uint8_t *data, size_t len,
				  void *user_data);

/**
 * 初始化 UART 设备
 * @param dev  UART 设备指针（DEVICE_DT_GET 获取）
 * @param cfg  配置参数，NULL 使用默认 115200-8-N-1
 * @return 0 成功，<0 错误码
 */
int plat_uart_init(const struct device *dev, const struct plat_uart_config *cfg);

/**
 * 轮询方式发送单字节
 * @return 0 成功，<0 错误码
 */
int plat_uart_poll_out(const struct device *dev, uint8_t byte);

/**
 * 轮询方式接收单字节（阻塞直到收到或超时）
 * @param timeout_ms 超时时间（ms），K_FOREVER 为无限等待
 * @return 0 成功，-EAGAIN 超时，<0 其他错误
 */
int plat_uart_poll_in(const struct device *dev, uint8_t *byte,
		      int32_t timeout_ms);

/**
 * 发送数据块（轮询）
 * @return 实际发送字节数，<0 错误码
 */
int plat_uart_write(const struct device *dev, const uint8_t *data, size_t len);

/**
 * 启用中断接收模式
 * @param dev     UART 设备
 * @param cb      接收回调（收到数据时调用）
 * @param user_data 用户数据指针
 * @return 0 成功，<0 错误码
 */
int plat_uart_rx_enable(const struct device *dev, plat_uart_rx_cb_t cb,
			void *user_data);

/**
 * 禁用中断接收模式
 */
int plat_uart_rx_disable(const struct device *dev);

/**
 * 发送数据块（中断/DMA 异步，预留）
 * @note 当前版本内部仍使用轮询发送，后续可切换为中断/DMA
 */
int plat_uart_write_async(const struct device *dev, const uint8_t *data,
			  size_t len);

#endif /* PLATFORM_PLAT_UART_H_ */
