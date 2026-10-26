/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：系统服务层 -> 网络协议栈（IPv4 + MQTT 单链路）
 *
 * 对上游网络协议栈和 MQTT 库的平台统一封装。
 * 蓝牙/蜂窝/TLS 预留，当前仅实现 IPv4 + MQTT 单链路。
 *
 * 平台差异：
 *   - esp32c6：WiFi STA + MQTT client
 *   - stm32f407/atm3405：预留接口（桩实现），实际链路后续接入
 */
#ifndef PLATFORM_PLAT_NET_H_
#define PLATFORM_PLAT_NET_H_

#include <stdbool.h>
#include <stdint.h>

/** 网络连接状态 */
enum plat_net_state {
	PLAT_NET_STATE_DISCONNECTED,	/**< 未连接 */
	PLAT_NET_STATE_CONNECTING,	/**< 连接中 */
	PLAT_NET_STATE_CONNECTED,	/**< 已连接 */
	PLAT_NET_STATE_ERROR,		/**< 错误 */
};

/** MQTT 消息接收回调 */
typedef void (*plat_net_mqtt_rx_cb_t)(const char *topic, uint16_t topic_len,
				      const uint8_t *payload, uint16_t payload_len,
				      void *user_data);

/** 网络状态变化回调 */
typedef void (*plat_net_state_cb_t)(enum plat_net_state state, void *user_data);

/**
 * 初始化网络协议栈
 * @return 0 成功，<0 错误码
 */
int plat_net_init(void);

/**
 * 启动网络连接（WiFi STA / 以太网 / 预留）
 * @param ssid     WiFi SSID（仅 WiFi 平台使用，其他平台传 NULL）
 * @param password WiFi 密码（仅 WiFi 平台使用，其他平台传 NULL）
 * @return 0 成功，<0 错误码
 */
int plat_net_connect(const char *ssid, const char *password);

/**
 * 断开网络连接
 */
int plat_net_disconnect(void);

/**
 * 查询当前网络状态
 */
enum plat_net_state plat_net_get_state(void);

/**
 * 注册网络状态变化回调
 */
int plat_net_register_state_cb(plat_net_state_cb_t cb, void *user_data);

/**
 * 初始化 MQTT 客户端并连接到 broker
 * @param broker_addr  broker 地址（如 "192.168.1.100" 或 "mqtt.example.com"）
 * @param broker_port  broker 端口（通常 1883）
 * @param client_id    客户端 ID
 * @param rx_cb        消息接收回调
 * @param user_data    用户数据指针
 * @return 0 成功，<0 错误码
 */
int plat_net_mqtt_connect(const char *broker_addr, uint16_t broker_port,
			  const char *client_id,
			  plat_net_mqtt_rx_cb_t rx_cb, void *user_data);

/**
 * 断开 MQTT 连接
 */
int plat_net_mqtt_disconnect(void);

/**
 * 发布 MQTT 消息
 * @param topic    主题
 * @param payload  消息内容
 * @param len      消息长度
 * @param qos      QoS 等级（0/1/2）
 * @return 0 成功，<0 错误码
 */
int plat_net_mqtt_publish(const char *topic, const uint8_t *payload,
			  uint16_t len, uint8_t qos);

/**
 * 订阅 MQTT 主题
 * @param topic 主题
 * @param qos   QoS 等级
 * @return 0 成功，<0 错误码
 */
int plat_net_mqtt_subscribe(const char *topic, uint8_t qos);

/**
 * 取消订阅 MQTT 主题
 */
int plat_net_mqtt_unsubscribe(const char *topic);

/**
 * MQTT 客户端是否已连接
 */
bool plat_net_mqtt_is_connected(void);

/**
 * 网络任务处理（需在主循环或独立线程中周期调用）
 * @note 非阻塞，内部处理 WiFi 事件、MQTT keepalive 等
 */
void plat_net_process(void);

#endif /* PLATFORM_PLAT_NET_H_ */
