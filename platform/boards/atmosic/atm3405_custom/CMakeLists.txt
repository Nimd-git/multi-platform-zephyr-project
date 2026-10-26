# SPDX-License-Identifier: Apache-2.0
#
# ATM3405 定制板构建脚本。
# 从 openair boards/atmosic/atm34evk/CMakeLists.txt 精简而来，
# 保留 no-TZ (no-SPE) 模式所需的编译选项。

if(CONFIG_TRUSTED_EXECUTION_SECURE)
  zephyr_compile_options(
    -DSECURE_MODE=1
  )
endif()

zephyr_compile_options_ifdef(CONFIG_ATM_NO_SPE -DCFG_NO_SPE=1)
