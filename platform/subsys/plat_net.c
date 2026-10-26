// SPDX-License-Identifier: Apache-2.0
/*
 * 架构图：系统服务层 -> 网络协议栈（IPv4 + MQTT 单链路）
 * 蓝牙/蜂窝/TLS 预留。
 *
 * 平台差异：
 *   - esp32c6：WiFi STA + MQTT client（完整实现）
 *   - stm32f407/atm3405：预留接口（桩实现），实际链路后续接入
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <string.h>

#include <platform/plat_net.h>

LOG_MODULE_REGISTER(plat_net);

#if defined(CONFIG_WIFI) && defined(CONFIG_MQTT_LIB) && defined(CONFIG_NET_IPV4)

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/mqtt.h>
#include <zephyr/net/net_ip.h>

/* ---- 内部状态 ----------------------------------------------------------- */

static enum plat_net_state net_state = PLAT_NET_STATE_DISCONNECTED;
static plat_net_state_cb_t state_cb;
static void *state_cb_user_data;

static struct mqtt_client mqtt_ctx;
static bool mqtt_connected;
static plat_net_mqtt_rx_cb_t mqtt_rx_cb;
static void *mqtt_rx_user_data;

static struct net_mgmt_event_callback wifi_cb;
static struct net_mgmt_event_callback ipv4_cb;

static K_SEM_DEFINE(wifi_connected_sem, 0, 1);
static K_SEM_DEFINE(ipv4_obtained_sem, 0, 1);

/* ---- 网络状态管理 -------------------------------------------------------- */

static void set_net_state(enum plat_net_state state)
{
	if (net_state != state) {
		net_state = state;
		if (state_cb != NULL) {
			state_cb(state, state_cb_user_data);
		}
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		LOG_INF("WiFi connected");
		k_sem_give(&wifi_connected_sem);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		LOG_INF("WiFi disconnected");
		set_net_state(PLAT_NET_STATE_DISCONNECTED);
		mqtt_connected = false;
		break;
	default:
		break;
	}
}

static void ipv4_mgmt_event_handler(struct net_mgmt_event_callback *cb,
				    uint64_t mgmt_event, struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
		LOG_INF("IPv4 address obtained");
		k_sem_give(&ipv4_obtained_sem);
		set_net_state(PLAT_NET_STATE_CONNECTED);
	}
}

/* ---- MQTT 事件处理 -------------------------------------------------------- */

static void mqtt_evt_handler(struct mqtt_client *client,
			     const struct mqtt_evt *evt)
{
	ARG_UNUSED(client);

	switch (evt->type) {
	case MQTT_EVT_CONNACK:
		if (evt->result == 0) {
			mqtt_connected = true;
			LOG_INF("MQTT connected");
		} else {
			LOG_ERR("MQTT connect failed: %d", evt->result);
			mqtt_connected = false;
		}
		break;

	case MQTT_EVT_DISCONNECT:
		mqtt_connected = false;
		LOG_INF("MQTT disconnected");
		break;

	case MQTT_EVT_PUBLISH: {
		const struct mqtt_publish_param *pub = &evt->param.publish;
		uint8_t payload[256];
		int len;

		len = mqtt_read_publish_payload(client, payload, sizeof(payload));
		if (len < 0) {
			len = 0;
		}

		if (mqtt_rx_cb != NULL) {
			mqtt_rx_cb(pub->message.topic.topic.utf8,
				   pub->message.topic.topic.size,
				   payload, (uint16_t)len,
				   mqtt_rx_user_data);
		}
		break;
	}

	case MQTT_EVT_PUBACK:
	case MQTT_EVT_SUBACK:
	case MQTT_EVT_UNSUBACK:
	case MQTT_EVT_PINGRESP:
		break;

	default:
		break;
	}
}

/* ---- 公共接口 ------------------------------------------------------------- */

int plat_net_init(void)
{
	net_mgmt_init_event_callback(&wifi_cb, wifi_mgmt_event_handler,
				     NET_EVENT_WIFI_CONNECT_RESULT |
				     NET_EVENT_WIFI_DISCONNECT_RESULT);
	net_mgmt_add_event_callback(&wifi_cb);

	net_mgmt_init_event_callback(&ipv4_cb, ipv4_mgmt_event_handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&ipv4_cb);

	mqtt_client_init(&mqtt_ctx);

	LOG_INF("Network stack initialized (WiFi + MQTT)");
	return 0;
}

int plat_net_connect(const char *ssid, const char *password)
{
	struct net_if *iface;
	struct wifi_connect_req_params params = { 0 };
	int ret;

	if (net_state == PLAT_NET_STATE_CONNECTED) {
		return 0;
	}

	iface = net_if_get_default();
	if (iface == NULL) {
		return -ENODEV;
	}

	set_net_state(PLAT_NET_STATE_CONNECTING);

	params.ssid = (const uint8_t *)ssid;
	params.ssid_length = strlen(ssid);
	params.psk = (const uint8_t *)password;
	params.psk_length = strlen(password);
	params.channel = WIFI_CHANNEL_ANY;
	params.security = WIFI_SECURITY_TYPE_PSK;

	ret = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &params, sizeof(params));
	if (ret != 0) {
		LOG_ERR("WiFi connect request failed: %d", ret);
		set_net_state(PLAT_NET_STATE_ERROR);
		return ret;
	}

	/* 等待 WiFi 连接和 IPv4 地址获取 */
	ret = k_sem_take(&wifi_connected_sem, K_SECONDS(30));
	if (ret != 0) {
		LOG_ERR("WiFi connect timeout");
		set_net_state(PLAT_NET_STATE_ERROR);
		return -ETIMEDOUT;
	}

	ret = k_sem_take(&ipv4_obtained_sem, K_SECONDS(30));
	if (ret != 0) {
		LOG_ERR("IPv4 address timeout");
		set_net_state(PLAT_NET_STATE_ERROR);
		return -ETIMEDOUT;
	}

	LOG_INF("Network connected");
	return 0;
}

int plat_net_disconnect(void)
{
	struct net_if *iface = net_if_get_default();
	int ret;

	if (iface == NULL) {
		return -ENODEV;
	}

	if (mqtt_connected) {
		(void)plat_net_mqtt_disconnect();
	}

	ret = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);
	if (ret != 0) {
		LOG_ERR("WiFi disconnect failed: %d", ret);
		return ret;
	}

	set_net_state(PLAT_NET_STATE_DISCONNECTED);
	return 0;
}

enum plat_net_state plat_net_get_state(void)
{
	return net_state;
}

int plat_net_register_state_cb(plat_net_state_cb_t cb, void *user_data)
{
	state_cb = cb;
	state_cb_user_data = user_data;
	return 0;
}

int plat_net_mqtt_connect(const char *broker_addr, uint16_t broker_port,
			  const char *client_id,
			  plat_net_mqtt_rx_cb_t rx_cb, void *user_data)
{
	struct sockaddr_in broker = { 0 };
	int ret;

	if (net_state != PLAT_NET_STATE_CONNECTED) {
		return -ENOTCONN;
	}

	if ((broker_addr == NULL) || (client_id == NULL)) {
		return -EINVAL;
	}

	broker.sin_family = AF_INET;
	broker.sin_port = htons(broker_port);
	ret = net_addr_pton(AF_INET, broker_addr, &broker.sin_addr);
	if (ret != 1) {
		LOG_ERR("Invalid broker address: %s", broker_addr);
		return -EINVAL;
	}

	mqtt_ctx.broker = &broker;
	mqtt_ctx.client_id.utf8 = (const uint8_t *)client_id;
	mqtt_ctx.client_id.size = strlen(client_id);
	mqtt_ctx.evt_cb = mqtt_evt_handler;
	mqtt_ctx.protocol_version = MQTT_VERSION_3_1_1;
	mqtt_ctx.keepalive = CONFIG_MQTT_KEEPALIVE;

	mqtt_rx_cb = rx_cb;
	mqtt_rx_user_data = user_data;

	ret = mqtt_connect(&mqtt_ctx);
	if (ret != 0) {
		LOG_ERR("mqtt_connect failed: %d", ret);
		return ret;
	}

	/* 等待 CONNACK */
	ret = k_sem_take(&wifi_connected_sem, K_SECONDS(10));
	if (ret != 0 && !mqtt_connected) {
		LOG_ERR("MQTT connect timeout");
		return -ETIMEDOUT;
	}

	return 0;
}

int plat_net_mqtt_disconnect(void)
{
	struct mqtt_disconnect_param param;

	memset(&param, 0, sizeof(param));
	int ret;

	if (!mqtt_connected) {
		return 0;
	}

#if defined(CONFIG_MQTT_VERSION_5_0)
	param.reason_code = MQTT_REASON_CODE_SUCCESS;
#endif
	ret = mqtt_disconnect(&mqtt_ctx, &param);
	if (ret == 0) {
		mqtt_connected = false;
	}
	return ret;
}

int plat_net_mqtt_publish(const char *topic, const uint8_t *payload,
			  uint16_t len, uint8_t qos)
{
	struct mqtt_publish_param param = { 0 };

	if (!mqtt_connected) {
		return -ENOTCONN;
	}
	if ((topic == NULL) || (payload == NULL)) {
		return -EINVAL;
	}

	param.message.topic.topic.utf8 = (const uint8_t *)topic;
	param.message.topic.topic.size = strlen(topic);
	param.message.topic.qos = qos;
	param.message.payload.data = (uint8_t *)payload;
	param.message.payload.len = len;
	param.message_id = 1;
	param.dup_flag = 0;
	param.retain_flag = 0;

	return mqtt_publish(&mqtt_ctx, &param);
}

int plat_net_mqtt_subscribe(const char *topic, uint8_t qos)
{
	struct mqtt_topic topics[] = {
		{
			.topic = {
				.utf8 = (const uint8_t *)topic,
				.size = strlen(topic),
			},
			.qos = qos,
		},
	};
	struct mqtt_subscription_list sub_list = {
		.list = topics,
		.list_count = ARRAY_SIZE(topics),
		.message_id = 1,
	};

	if (!mqtt_connected) {
		return -ENOTCONN;
	}

	return mqtt_subscribe(&mqtt_ctx, &sub_list);
}

int plat_net_mqtt_unsubscribe(const char *topic)
{
	struct mqtt_topic topics[] = {
		{
			.topic = {
				.utf8 = (const uint8_t *)topic,
				.size = strlen(topic),
			},
		},
	};
	struct mqtt_subscription_list unsub_list = {
		.list = topics,
		.list_count = ARRAY_SIZE(topics),
		.message_id = 1,
	};

	if (!mqtt_connected) {
		return -ENOTCONN;
	}

	return mqtt_unsubscribe(&mqtt_ctx, &unsub_list);
}

bool plat_net_mqtt_is_connected(void)
{
	return mqtt_connected;
}

void plat_net_process(void)
{
	if (mqtt_connected) {
		/* MQTT keepalive 和输入数据处理 */
		(void)mqtt_live(&mqtt_ctx);
		(void)mqtt_input(&mqtt_ctx);
	}
}

#else /* !CONFIG_WIFI || !CONFIG_MQTT_LIB || !CONFIG_NET_IPV4 */

/* ---- 桩实现（stm32f407 / atm3405 等平台） -------------------------------- */

static enum plat_net_state stub_state = PLAT_NET_STATE_DISCONNECTED;

int plat_net_init(void)
{
	LOG_WRN("Network stack not available on this platform (stub)");
	return 0;
}

int plat_net_connect(const char *ssid, const char *password)
{
	ARG_UNUSED(ssid);
	ARG_UNUSED(password);
	LOG_WRN("Network connect not supported (stub)");
	return -ENOTSUP;
}

int plat_net_disconnect(void)
{
	return -ENOTSUP;
}

enum plat_net_state plat_net_get_state(void)
{
	return stub_state;
}

int plat_net_register_state_cb(plat_net_state_cb_t cb, void *user_data)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);
	return 0;
}

int plat_net_mqtt_connect(const char *broker_addr, uint16_t broker_port,
			  const char *client_id,
			  plat_net_mqtt_rx_cb_t rx_cb, void *user_data)
{
	ARG_UNUSED(broker_addr);
	ARG_UNUSED(broker_port);
	ARG_UNUSED(client_id);
	ARG_UNUSED(rx_cb);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
}

int plat_net_mqtt_disconnect(void)
{
	return -ENOTSUP;
}

int plat_net_mqtt_publish(const char *topic, const uint8_t *payload,
			  uint16_t len, uint8_t qos)
{
	ARG_UNUSED(topic);
	ARG_UNUSED(payload);
	ARG_UNUSED(len);
	ARG_UNUSED(qos);
	return -ENOTSUP;
}

int plat_net_mqtt_subscribe(const char *topic, uint8_t qos)
{
	ARG_UNUSED(topic);
	ARG_UNUSED(qos);
	return -ENOTSUP;
}

int plat_net_mqtt_unsubscribe(const char *topic)
{
	ARG_UNUSED(topic);
	return -ENOTSUP;
}

bool plat_net_mqtt_is_connected(void)
{
	return false;
}

void plat_net_process(void)
{
	/* 空实现 */
}

#endif /* CONFIG_WIFI && CONFIG_MQTT_LIB && CONFIG_NET_IPV4 */
