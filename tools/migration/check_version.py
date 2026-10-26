#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""
版本兼容与适配 - 迁移工具链（骨架）

读取上游 zephyr/VERSION，输出平台兼容性结论，对应架构图左侧
“版本兼容与适配 -> 迁移工具链 -> 版本检查脚本”。

用法（工作区根目录）:
    python tools/migration/check_version.py
    python tools/migration/check_version.py --minimum 3 7
"""

import argparse
import sys
from pathlib import Path


# 架构图“版本兼容矩阵”：各 LTS/特性版本与当前平台层的兼容状态
COMPAT_MATRIX = [
    ("v1.14 LTS", "API 差异大，不兼容"),
    ("v2.7",       "兼容层适配"),
    ("v3.0",       "需迁移"),
    ("v3.5",       "部分废弃"),
    ("v3.7 LTS",   "向前兼容（基线）"),
    ("v4.1",       "完全兼容"),
    ("v4.4",       "完全兼容（当前开发基线）"),
]

# 平台层最低支持基线：v3.7 LTS（5 年长期支持，向前兼容）
MIN_MAJOR = 3
MIN_MINOR = 7


def parse_version(version_file: Path):
    values = {}
    for line in version_file.read_text(encoding="utf-8").splitlines():
        if "=" in line:
            key, val = line.split("=", 1)
            values[key.strip()] = val.strip()
    return (
        int(values["VERSION_MAJOR"]),
        int(values["VERSION_MINOR"]),
        int(values["PATCHLEVEL"]),
    )


def main():
    parser = argparse.ArgumentParser(description="Zephyr 平台版本兼容性检查")
    parser.add_argument("--minimum", nargs=2, type=int, metavar=("MAJOR", "MINOR"),
                        default=(MIN_MAJOR, MIN_MINOR))
    parser.add_argument("--workspace", default=None,
                        help="工作区根目录（默认按脚本位置推导）")
    args = parser.parse_args()

    root = Path(args.workspace) if args.workspace else Path(__file__).resolve().parents[2]
    version_file = root / "zephyr" / "VERSION"
    if not version_file.is_file():
        print(f"[ERROR] 未找到 {version_file}", file=sys.stderr)
        return 2

    major, minor, patch = parse_version(version_file)
    print(f"检测到 Zephyr 版本: v{major}.{minor}.{patch}")
    print("版本兼容矩阵:")
    for name, status in COMPAT_MATRIX:
        print(f"  {name:12s} {status}")

    min_major, min_minor = args.minimum
    if (major, minor) < (min_major, min_minor):
        print(f"[FAIL] 平台层要求 >= v{min_major}.{min_minor}，当前版本过低。")
        return 1

    print(f"[ OK ] 满足平台最低基线 v{min_major}.{min_minor}，可启用完整平台层。")
    print("[提示] 旧 Kconfig 选项迁移见 platform/compat/Kconfig.compat；")
    print("       API 版本隔离宏见 platform/include/platform/plat_version.h。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
