# SPDX-License-Identifier: Apache-2.0
#
# 烧录/调试：J-Link（SWD）。J-Link 设备名取 Flash+引脚代码（与
# nucleo_u5a5zj_q 的 STM32U5A5ZJ 命名规则一致），对应 STM32U5G9BJY6Q。

board_runner_args(jlink "--device=STM32U5G9BJ" "--speed=4000" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
