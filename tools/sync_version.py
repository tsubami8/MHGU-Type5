#!/usr/bin/env python3
"""把 README 里的版本号同步成给定 tag。

用法（CI 里由 workflow 调用）：
    python tools/sync_version.py v1.0.1

同步内容：
  1. 一级标题末尾的版本号：``# 某某 v1.0.0`` -> ``# 某某 v1.0.1``
  2. 发行包文件名里的版本号：``MHGU-XXX-v1.0.0.zip`` -> ``MHGU-XXX-v1.0.1.zip``
"""

import re
import sys
from pathlib import Path

README = Path("README.md")
# v1.0.1 / 1.0.1 / v1.1.0-beta.1 / v2.0.0-rc.2
TAG_RE = re.compile(r"^v?(\d+\.\d+(?:\.\d+)?(?:[-.][0-9A-Za-z.]+)?)$")


def main(argv):
    if len(argv) != 2:
        print("用法: python tools/sync_version.py <tag>", file=sys.stderr)
        return 2
    tag = argv[1].strip()
    m = TAG_RE.match(tag)
    if not m:
        print(f"跳过：{tag!r} 不是版本号格式的 tag", file=sys.stderr)
        return 0
    display = tag if tag.startswith("v") else "v" + tag
    pattern = re.escape(display)

    text = README.read_text(encoding="utf-8")
    original = text

    # 1) 一级标题：把结尾的旧版本号换成新版本号
    text = re.sub(
        r"(?m)^(#\s+.*?)\s*v?\d+\.\d+(?:\.\d+)?(?:[-.][0-9A-Za-z.]+)?\s*$",
        lambda mo: f"{mo.group(1)} {display}",
        text,
        count=1,
    )

    # 2) 发行包文件名：MHGU-XXX-<版本>.zip
    text = re.sub(
        r"(MHGU-[A-Za-z0-9._-]+?)-v?\d+\.\d+(?:\.\d+)?(?:[-.][0-9A-Za-z.]+)?\.zip",
        lambda mo: f"{mo.group(1)}-{display}.zip",
        text,
    )

    if text == original:
        print(f"README 已是 {display}，无需改动")
        return 0

    README.write_text(text, encoding="utf-8")
    changed = [
        f"  - {a.strip()}\n    + {b.strip()}"
        for a, b in zip(original.splitlines(), text.splitlines())
        if a != b
    ]
    print(f"README 版本号已同步为 {display}（{len(changed)} 行）")
    for line in changed:
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
