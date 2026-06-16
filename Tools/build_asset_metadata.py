#!/usr/bin/env python3
"""Build the manifest that couples one MCU firmware image to assets.bin."""

from __future__ import annotations

import argparse
import hashlib
import json
import struct
import zlib
from pathlib import Path


MAGIC = 0x41444854
VERSION = 2
PACKAGE_ID_SIZE = 16
MANIFEST_SIZE = 32


def build_manifest(data: bytes) -> tuple[bytes, dict[str, object]]:
    if not data:
        raise ValueError("asset image is empty")

    crc = zlib.crc32(data) & 0xFFFFFFFF
    digest = hashlib.sha256(data).digest()
    package_id = digest[:PACKAGE_ID_SIZE]
    manifest = struct.pack("<IIII", MAGIC, VERSION, len(data), crc) + package_id
    if len(manifest) != MANIFEST_SIZE:
        raise AssertionError("unexpected manifest size")

    metadata = {
        "magic": f"0x{MAGIC:08X}",
        "version": VERSION,
        "data_length": len(data),
        "crc32": f"0x{crc:08X}",
        "package_id": package_id.hex().upper(),
        "sha256": digest.hex().upper(),
    }
    return manifest, metadata


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("assets", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("metadata", type=Path)
    args = parser.parse_args()

    data = args.assets.read_bytes()
    manifest, metadata = build_manifest(data)
    args.manifest.write_bytes(manifest)
    args.metadata.write_text(json.dumps(metadata, indent=2) + "\n", encoding="ascii")
    print(
        "Asset package: "
        f"{metadata['data_length']} bytes, {metadata['crc32']}, "
        f"id={metadata['package_id']}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
