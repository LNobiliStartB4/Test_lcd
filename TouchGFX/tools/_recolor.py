"""One-shot recolor/rename of Tabler icons for Screen7 redesign.

Run from anywhere:  python <path-to-this-file>
Targets:  ../assets/images relative to this file.
"""
from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageDraw

HERE = (Path(__file__).resolve().parent.parent / "assets" / "images").resolve()


def recolor_alpha_mask(src: Path, dst: Path, rgb: tuple[int, int, int], size: tuple[int, int] | None = None) -> None:
    """Replace all non-transparent pixels with `rgb`, keep alpha, optionally resize."""
    img = Image.open(src).convert("RGBA")
    if size is not None and img.size != size:
        img = img.resize(size, Image.LANCZOS)
    pixels = img.load()
    r, g, b = rgb
    for y in range(img.height):
        for x in range(img.width):
            _, _, _, a = pixels[x, y]
            if a > 0:
                pixels[x, y] = (r, g, b, a)
    img.save(dst, "PNG")
    sz = f" @ {img.size[0]}x{img.size[1]}" if size else ""
    print(f"  recolored {src.name} -> {dst.name} (#{r:02X}{g:02X}{b:02X}){sz}")


def resize_only(src: Path, dst: Path, size: tuple[int, int]) -> None:
    img = Image.open(src).convert("RGBA")
    if img.size != size:
        img = img.resize(size, Image.LANCZOS)
    img.save(dst, "PNG")
    print(f"  resized {src.name} -> {dst.name} ({size[0]}x{size[1]})")


def rename(src: Path, dst: Path) -> None:
    if src.resolve() == dst.resolve():
        return
    if dst.exists():
        dst.unlink()
    src.rename(dst)
    print(f"  renamed {src.name} -> {dst.name}")


def make_pill(dst: Path, width: int, height: int, rgb: tuple[int, int, int]) -> None:
    img = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    radius = height // 2
    draw.rounded_rectangle((0, 0, width - 1, height - 1), radius=radius, fill=(*rgb, 255))
    img.save(dst, "PNG")
    print(f"  generated {dst.name} ({width}x{height}, #{rgb[0]:02X}{rgb[1]:02X}{rgb[2]:02X})")


def main() -> None:
    print("Recoloring icons in", HERE)

    # trash: outline black -> red #E24B4A
    src = HERE / "trash.png"
    if src.exists():
        recolor_alpha_mask(src, HERE / "trash_red.png", (0xE2, 0x4B, 0x4A))
        src.unlink()

    # arrow-narrow-left: rename only, stays black for white button
    src = HERE / "arrow-narrow-left.png"
    if src.exists():
        rename(src, HERE / "arrow_left_black.png")

    # alert-triangle (outline): recolor to white as fallback; user may
    # overwrite with the "alert-triangle-filled" variant later
    src = HERE / "alert-triangle.png"
    if src.exists():
        recolor_alpha_mask(src, HERE / "alert_triangle_white.png", (0xFF, 0xFF, 0xFF))
        src.unlink()

    # If user later drops alert-triangle-filled.png, recolor it on top
    src = HERE / "alert-triangle-filled.png"
    if src.exists():
        recolor_alpha_mask(src, HERE / "alert_triangle_white.png", (0xFF, 0xFF, 0xFF))
        src.unlink()
        print("  (overwrote alert_triangle_white.png with filled variant)")

    # Pill rounded background: 200x22 red
    make_pill(HERE / "pill_red.png", 200, 22, (0xE2, 0x4B, 0x4A))

    print("Done.")


if __name__ == "__main__":
    main()
