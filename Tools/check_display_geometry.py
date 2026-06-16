#!/usr/bin/env python3
"""Fail when TouchGFX project geometry drifts from the physical 480x320 panel."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


WIDTH = 480
HEIGHT = 320
PARTIAL_BLOCK_BYTES = 12800


class GeometryError(RuntimeError):
    pass


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise GeometryError(message)


def check_project(root: Path, require_generated: bool = True) -> None:
    ioc = (root / "Display_test_prova.ioc").read_text(encoding="utf-8")
    _require(
        f"tgfx_custom_width={WIDTH}" in ioc,
        f"IOC TouchGFX width must be {WIDTH}",
    )
    _require(
        f"tgfx_custom_height={HEIGHT}" in ioc,
        f"IOC TouchGFX height must be {HEIGHT}",
    )
    _require(
        f"tgfx_block_size={PARTIAL_BLOCK_BYTES}" in ioc,
        f"IOC partial framebuffer block must be {PARTIAL_BLOCK_BYTES} bytes",
    )

    designer = json.loads(
        (root / "TouchGFX" / "Display_test.touchgfx").read_text(encoding="utf-8")
    )
    resolution = designer["Application"]["Resolution"]
    _require(
        resolution == {"Width": WIDTH, "Height": HEIGHT},
        f"Designer resolution is {resolution}, expected {WIDTH}x{HEIGHT}",
    )

    template = json.loads(
        (root / "TouchGFX" / "ApplicationTemplate.touchgfx.part").read_text(
            encoding="utf-8"
        )
    )
    available = template["Application"]["AvailableResolutions"]
    _require(
        {"Width": WIDTH, "Height": HEIGHT} in available,
        f"TouchGFX template does not offer {WIDTH}x{HEIGHT}",
    )

    if require_generated:
        generated = (
            root
            / "TouchGFX"
            / "target"
            / "generated"
            / "TouchGFXConfiguration.cpp"
        ).read_text(encoding="utf-8")
        dimensions = re.search(
            r"static\s+TouchGFXHAL\s+hal\([^;]*,\s*(\d+)\s*,\s*(\d+)\s*\);",
            generated,
        )
        _require(dimensions is not None, "generated HAL dimensions not found")
        actual = tuple(int(value) for value in dimensions.groups())
        _require(
            actual == (WIDTH, HEIGHT),
            f"generated HAL is {actual[0]}x{actual[1]}, expected {WIDTH}x{HEIGHT}",
        )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="project root",
    )
    parser.add_argument(
        "--skip-generated",
        action="store_true",
        help="check source configuration before running TouchGFX generation",
    )
    args = parser.parse_args()

    try:
        check_project(args.root.resolve(), not args.skip_generated)
    except (GeometryError, KeyError, json.JSONDecodeError) as error:
        print(f"DISPLAY GEOMETRY ERROR: {error}")
        return 1

    print(f"Display geometry OK: {WIDTH}x{HEIGHT}, block={PARTIAL_BLOCK_BYTES}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
