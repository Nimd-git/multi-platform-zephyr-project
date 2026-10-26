/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file plat_uwb.h
 * @brief UWB 中间件：NXP SR040 UWB 芯片封装（UCI 协议栈）
 *
 * 依赖 nxp-uwbiot Zephyr 模块（modules/uwb/uwbiot）。
 * 板级接线：在 overlay 中声明如下节点（引脚按实际接线修改）：
 *
 *   &spiX {
 *       nxp_uwb_device: nxp_uwb_device@0 {
 *           compatible = "nxp,uwb-sr040-device";
 *           reg = <0>;
 *           spi-max-frequency = <1000000>;
 *           uwb-irq-gpios   = <&gpioX pin GPIO_ACTIVE_HIGH>;
 *           uwb-rdy-gpios   = <&gpioX pin GPIO_ACTIVE_HIGH>;
 *           uwb-reset-gpios = <&gpioX pin GPIO_ACTIVE_LOW>;
 *       };
 *   };
 *
 * 节点存在时 plat_uwb 真实工作；未声明时各接口返回 -ENODEV。
 */

#ifndef PLATFORM_PLAT_UWB_H_
#define PLATFORM_PLAT_UWB_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/** UWB 通知回调：op_type 为 eNotificationType 原始值，data/len 为通知负载 */
typedef void (*plat_uwb_notif_cb_t)(uint16_t op_type, void *data, size_t len, void *user_data);

/** SR040 设备信息（UCI GET_DEVICE_INFO） */
struct plat_uwb_dev_info {
	uint16_t uci_version;  /**< UCI 通用版本 */
	uint8_t mac_major;     /**< MAC 主版本 */
	uint8_t mac_minor;     /**< MAC 次版本 */
	uint8_t phy_major;     /**< PHY 主版本 */
	uint8_t phy_minor;     /**< PHY 次版本 */
	uint8_t fw_major;      /**< 固件主版本 */
	uint8_t fw_minor;      /**< 固件次版本 */
	uint8_t fw_patch;      /**< 固件补丁版本 */
	uint8_t mw_major;      /**< 中间件主版本 */
	uint8_t mw_minor;      /**< 中间件次版本 */
	char dev_name[33];     /**< 设备名（NUL 结尾） */
	char serial_no[17];    /**< 序列号（NUL 结尾） */
};

/**
 * @brief 注册 UWB 通知回调（init 之前调用生效）
 */
void plat_uwb_set_notif_cb(plat_uwb_notif_cb_t cb, void *user_data);

/**
 * @brief 初始化 UWB 协议栈（上电 + UCI 通道建立）
 */
int plat_uwb_init(void);

/**
 * @brief 反初始化 UWB 协议栈
 */
int plat_uwb_deinit(void);

/**
 * @brief 读取设备信息（版本/序列号等）
 */
int plat_uwb_get_device_info(struct plat_uwb_dev_info *info);

/**
 * @brief 创建并启动测距会话
 *
 * @param session_id 会话 ID（1..255）
 * @param session_handle 输出会话句柄
 * @retval 0 成功
 */
int plat_uwb_session_start(uint32_t session_id, uint32_t *session_handle);

/**
 * @brief 停止测距会话
 */
int plat_uwb_session_stop(uint32_t session_handle);

/**
 * @brief 销毁测距会话
 */
int plat_uwb_session_deinit(uint32_t session_handle);

#endif /* PLATFORM_PLAT_UWB_H_ */
