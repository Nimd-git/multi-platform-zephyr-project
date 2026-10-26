# SPDX-License-Identifier: Apache-2.0
#
# 抑制 DTC simple_bus_reg 警告：ATM34xx 分区地址由 Kconfig 计算得出，
# 分区 unit-address 与 reg 首地址不完全匹配。

list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
