from pathlib import Path
import re

from PIL import Image, ImageEnhance, ImageFilter


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "assets" / "bottom_penguin_source.png"
PREVIEW = ROOT / "assets" / "bottom_banner_480x200.png"
HEADER = ROOT / "Hardware" / "bottom_banner_data.h"
QSPI_BIN = ROOT / "assets" / "bottom_banner_qspi.bin"
WIDTH = 480
HEIGHT = 200


def fit(image: Image.Image, width: int, height: int) -> Image.Image:
    scale = max(width / image.width, height / image.height)
    resized = image.resize(
        (round(image.width * scale), round(image.height * scale)),
        Image.Resampling.LANCZOS,
    )
    left = (resized.width - width) // 2
    top = (resized.height - height) // 2
    return resized.crop((left, top, left + width, top + height))


def create_banner(source: Image.Image) -> Image.Image:
    background = fit(source, WIDTH, HEIGHT).filter(ImageFilter.GaussianBlur(18))
    background = ImageEnhance.Color(background).enhance(0.72)
    background = ImageEnhance.Brightness(background).enhance(0.62).convert("RGBA")

    shade = Image.new("RGBA", (WIDTH, HEIGHT), (0, 0, 0, 0))
    shade_pixels = shade.load()
    for x in range(WIDTH):
        alpha = max(0, min(118, round((x - 120) * 118 / 220)))
        for y in range(HEIGHT):
            shade_pixels[x, y] = (6, 25, 34, alpha)
    background.alpha_composite(shade)

    penguin_crop = source.crop((0, 70, source.width, 355))
    penguin = fit(penguin_crop, 190, HEIGHT).convert("RGBA")
    mask = Image.new("L", penguin.size, 255)
    mask_pixels = mask.load()
    for x in range(penguin.width):
        edge = min(x, penguin.width - 1 - x)
        alpha = min(255, edge * 28)
        for y in range(penguin.height):
            mask_pixels[x, y] = alpha
    background.paste(penguin, (8, 0), mask)
    return background.convert("RGB")


def rgb565(red: int, green: int, blue: int) -> int:
    return ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3)


def format_array(values: list[int], width: int, fmt: str) -> str:
    lines = []
    for start in range(0, len(values), width):
        chunk = values[start : start + width]
        lines.append("    " + ", ".join(format(value, fmt) for value in chunk) + ",")
    return "\n".join(lines)


def write_header(image: Image.Image) -> None:
    palette, pixels = quantize_banner(image)

    content = f"""#ifndef BOTTOM_BANNER_DATA_H
#define BOTTOM_BANNER_DATA_H

#include <stdint.h>

#define BOTTOM_BANNER_WIDTH  ({WIDTH}U)
#define BOTTOM_BANNER_HEIGHT ({HEIGHT}U)

static const uint16_t g_bottom_banner_palette[256] = {{
{format_array(palette, 8, '#06x')}
}};

static const uint8_t g_bottom_banner_pixels[BOTTOM_BANNER_WIDTH * BOTTOM_BANNER_HEIGHT] = {{
{format_array(pixels, 24, '#04x')}
}};

#endif
"""
    HEADER.write_text(content, encoding="ascii", newline="\n")


def quantize_banner(image: Image.Image) -> tuple[list[int], list[int]]:
    indexed = image.quantize(
        colors=256,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.FLOYDSTEINBERG,
    )
    palette_bytes = indexed.getpalette()[: 256 * 3]
    palette = [
        rgb565(*palette_bytes[index : index + 3])
        for index in range(0, len(palette_bytes), 3)
    ]
    pixels = list(indexed.getdata())

    return palette, pixels


def write_qspi_bin(image: Image.Image) -> None:
    palette, pixels = quantize_banner(image)
    payload = bytearray()
    payload += b"YFB1"
    payload += WIDTH.to_bytes(2, "little")
    payload += HEIGHT.to_bytes(2, "little")
    payload += bytes(8)
    payload += bytes(pixels)
    for color in palette:
        payload += color.to_bytes(2, "little")
    QSPI_BIN.write_bytes(payload)


def render_text_preview(image: Image.Image) -> Image.Image:
    hal_entry = (ROOT / "src" / "hal_entry.c").read_text(encoding="utf-8")
    font_block = hal_entry.split("static const uint8_t font_8x8", 1)[1].split("};", 1)[0]
    font = [int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", font_block)]
    preview = image.copy()
    pixels = preview.load()

    def draw_string(x: int, y: int, text: str, color: tuple[int, int, int], scale: int) -> None:
        for character in text:
            glyph = font[(ord(character) - 32) * 8 : (ord(character) - 31) * 8]
            for row, bits in enumerate(glyph):
                for column in range(8):
                    if bits & (0x80 >> column):
                        for dy in range(scale):
                            for dx in range(scale):
                                pixels[x + column * scale + dx, y + row * scale + dy] = color
            x += 8 * scale

    scale = 3
    right_edge = 466
    title_x = right_edge - 11 * 8 * scale
    test_x = right_edge - 4 * 8 * scale
    draw_string(title_x + 2, 62, "YOLOFASTEST", (16, 16, 16), scale)
    draw_string(test_x + 2, 98, "TEST", (16, 16, 16), scale)
    draw_string(title_x, 60, "YOLOFASTEST", (255, 255, 255), scale)
    draw_string(test_x, 96, "TEST", (255, 255, 255), scale)
    return preview


def main() -> None:
    source = Image.open(SOURCE).convert("RGB")
    banner = create_banner(source)
    write_header(banner)
    write_qspi_bin(banner)
    render_text_preview(banner).save(PREVIEW, optimize=True)


if __name__ == "__main__":
    main()
