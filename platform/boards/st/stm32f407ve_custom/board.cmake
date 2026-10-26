# SPDX-License-Identifier: Apache-2.0
#
# 烧录/调试：J-Link（SWD）。

board_runner_args(jlink "--device=STM32F407VE" "--speed=4000" "--reset-after-load")

# keep first
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
