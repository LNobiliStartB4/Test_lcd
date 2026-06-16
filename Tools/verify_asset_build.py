#!/usr/bin/env python3
"""Verify packaged assets and ensure the MCU HEX has no external-flash records."""

from __future__ import annotations

import argparse
from pathlib import Path

from build_asset_metadata import build_manifest


EXTERNAL_BASE = 0x90000000


def hex_addresses(path: Path):
    upper = 0
    for line_number, line in enumerate(path.read_text(encoding="ascii").splitlines(), 1):
        if not line.startswith(":"):
            raise ValueError(f"{path}:{line_number}: invalid Intel HEX record")
        raw = bytes.fromhex(line[1:])
        length = raw[0]
        address = (raw[1] << 8) | raw[2]
        record_type = raw[3]
        payload = raw[4:4 + length]
        if record_type == 0x04:
            upper = int.from_bytes(payload, "big") << 16
        elif record_type == 0x00:
            yield upper + address
        elif record_type == 0x01:
            return


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--assets", required=True, type=Path)
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--internal-hex", required=True, type=Path)
    parser.add_argument("--embedded-manifest", type=Path)
    args = parser.parse_args()

    expected, metadata = build_manifest(args.assets.read_bytes())
    actual = args.manifest.read_bytes()
    if actual != expected:
        raise SystemExit("asset manifest does not match assets.bin")
    if (args.embedded_manifest is not None and
            args.embedded_manifest.read_bytes() != expected):
        raise SystemExit("MCU AssetExpectedSection does not match assets.bin")

    external_records = [
        address for address in hex_addresses(args.internal_hex)
        if address >= EXTERNAL_BASE
    ]
    if external_records:
        raise SystemExit(
            f"internal HEX contains external address 0x{external_records[0]:08X}"
        )

    print(
        f"Asset build verified: {metadata['data_length']} bytes, "
        f"id={metadata['package_id']}; internal HEX contains MCU flash only"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
