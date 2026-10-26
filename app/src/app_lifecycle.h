/* SPDX-License-Identifier: Apache-2.0 */
/*
 * 架构图：应用开发层 -> 应用框架 -> 应用生命周期管理
 */
#ifndef APP_LIFECYCLE_H_
#define APP_LIFECYCLE_H_

/** main() 入口调用：标志内核/平台初始化全部完成，应用主循环开始 */
void app_lifecycle_boot_complete(void);

#endif /* APP_LIFECYCLE_H_ */
