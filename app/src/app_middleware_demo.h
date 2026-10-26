/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：应用开发层 -> 中间件演示入口
 *
 * 本文件演示平台中间件的使用：
 *   - 存储中间件：KV 键值存取（plat_kv_*，基于 settings）
 *   - 数据中间件：JSON 编解码 + CBOR 流式编码（plat_data_*）
 *   - 通信中间件：Modbus RTU 主站读保持寄存器（plat_modbus_*）
 *   - UWB 中间件：SR040 初始化/设备信息/测距会话（plat_uwb_*）
 */
#ifndef APP_MIDDLEWARE_DEMO_H_
#define APP_MIDDLEWARE_DEMO_H_

/**
 * 启动中间件演示（KV / JSON+CBOR / Modbus / UWB）
 * 在 main() 中调用一次即可。
 */
void app_middleware_demo_start(void);

#endif /* APP_MIDDLEWARE_DEMO_H_ */
