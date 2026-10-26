# SPDX-License-Identifier: Apache-2.0
#
# 烧录/调试：atmisp runner（基于 Atmosic OpenOCD）。
# atmisp 使用 openair HAL 的 OpenOCD 脚本和 fast_load 二进制。

if(CONFIG_SOC_ATM34XX_5)
  set(CHIP_REV rev-5)
endif()

set(PLAT_DIR "${ZEPHYR_OPENAIR_MODULE_DIR}/modules/hal_atmosic/ATM34xx")
board_runner_args(atmisp "--rram")
board_runner_args(atmisp "--atm_openocd_search=${PLAT_DIR}/${CHIP_REV}/openocd")
board_runner_args(atmisp "--fast_load_bin=${PLAT_DIR}/${CHIP_REV}/openocd/fast_load/fast_load.bin")

# atmfastload runner（platform 模块自研，J-Link fast_load 协议）。
# J-Link FLM loadfile 对 ATM34 RRAM 擦除失败，标准 jlink runner 不可用；
# atmfastload 复现 openair fast_load 流程，作为本板默认烧录 runner。
board_runner_args(atmfastload "--device=ATM3405-WQK")
board_runner_args(atmfastload "--fast-load-bin=${PLAT_DIR}/${CHIP_REV}/openocd/fast_load/fast_load.bin")
board_finalize_runner_args(atmfastload)
board_set_flasher_ifnset(atmfastload)

include(${ZEPHYR_OPENAIR_MODULE_DIR}/boards/common/atmisp.board.cmake)
