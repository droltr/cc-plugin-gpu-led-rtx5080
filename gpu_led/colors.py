"""Colour helpers matching cc-plugin-thermal-rgb's OKLCH gradient."""

import math
from dataclasses import dataclass

RGB = tuple[int, int, int]


def scale_rgb(rgb: RGB, brightness_pct: float) -> RGB:
    """Limit the RGB command level; this is not a hardware brightness register."""
    level = min(100.0, max(0.0, brightness_pct)) / 100.0
    return tuple(round(channel * level) for channel in rgb)  # type: ignore[return-value]


@dataclass(frozen=True)
class ColorStop:
    temp_c: float
    rgb: RGB


def hex_to_rgb(value: str) -> RGB:
    text = value.strip().lstrip("#")
    if len(text) != 6 or any(char not in "0123456789abcdefABCDEF" for char in text):
        raise ValueError(f"invalid hex colour: {value!r}")
    return int(text[0:2], 16), int(text[2:4], 16), int(text[4:6], 16)


def color_at(temp_c: float, stops: tuple[ColorStop, ...]) -> RGB:
    ordered = sorted(stops, key=lambda stop: stop.temp_c)
    if not ordered:
        raise ValueError("at least one colour stop is required")
    if temp_c <= ordered[0].temp_c:
        return ordered[0].rgb
    if temp_c >= ordered[-1].temp_c:
        return ordered[-1].rgb
    for low, high in zip(ordered, ordered[1:], strict=False):
        if low.temp_c <= temp_c <= high.temp_c:
            ratio = (temp_c - low.temp_c) / (high.temp_c - low.temp_c)
            return mix_oklch(low.rgb, high.rgb, ratio)
    raise RuntimeError("temperature fell outside sorted colour stops")


def _srgb_to_linear(value: float) -> float:
    return value / 12.92 if value <= 0.04045 else ((value + 0.055) / 1.055) ** 2.4


def _linear_to_srgb(value: float) -> float:
    return 12.92 * value if value <= 0.0031308 else 1.055 * value ** (1 / 2.4) - 0.055


def _to_oklch(rgb: RGB) -> tuple[float, float, float]:
    r, g, b = (_srgb_to_linear(channel / 255) for channel in rgb)
    l = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b
    m = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b
    s = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b
    l, m, s = (math.copysign(abs(value) ** (1 / 3), value) for value in (l, m, s))
    lightness = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s
    a = 1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s
    b_value = 0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s
    return lightness, math.hypot(a, b_value), math.degrees(math.atan2(b_value, a)) % 360


def _from_oklch(lightness: float, chroma: float, hue: float) -> RGB:
    a = chroma * math.cos(math.radians(hue))
    b = chroma * math.sin(math.radians(hue))
    l = (lightness + 0.3963377774 * a + 0.2158037573 * b) ** 3
    m = (lightness - 0.1055613458 * a - 0.0638541728 * b) ** 3
    s = (lightness - 0.0894841775 * a - 1.2914855480 * b) ** 3
    channels = (
        4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s,
        -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s,
        -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s,
    )
    return tuple(  # type: ignore[return-value]
        round(min(1.0, max(0.0, _linear_to_srgb(min(1.0, max(0.0, value))))) * 255)
        for value in channels
    )


def mix_oklch(first: RGB, second: RGB, ratio: float) -> RGB:
    ratio = min(1.0, max(0.0, ratio))
    l1, c1, h1 = _to_oklch(first)
    l2, c2, h2 = _to_oklch(second)
    if c1 < 1e-4:
        h1 = h2
    if c2 < 1e-4:
        h2 = h1
    delta = ((h2 - h1 + 180) % 360) - 180
    hue = (h1 + delta * ratio) % 360
    return _from_oklch(l1 + (l2 - l1) * ratio, c1 + (c2 - c1) * ratio, hue)
