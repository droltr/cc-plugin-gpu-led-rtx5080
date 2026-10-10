"""Configuration for the CoolerControl GPU LED integration."""

import json
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from .colors import ColorStop, hex_to_rgb

# The thermal-rgb scale is 50/60/70/80 °C. Its colour stops are preserved and
# linearly mapped onto the RTX 5080's practical 40–88 °C GPU range.
DEFAULT_STOPS = (
    {"temp_c": 40, "color": "#2060ff"},
    {"temp_c": 56, "color": "#ffd000"},
    {"temp_c": 72, "color": "#ff7a00"},
    {"temp_c": 88, "color": "#ff0000"},
)


class ConfigError(ValueError):
    """Raised when a plugin configuration is unsafe or malformed."""


@dataclass(frozen=True)
class Config:
    coolercontrol_url: str = "http://127.0.0.1:11987"
    coolercontrol_token: str = ""
    temp_device_uid: str = ""
    gpu_name: str = "NVIDIA GeForce RTX 5080"
    temp_name: str = "GPU Temp"
    driver_version: str = ""
    controller: str = "spectra-write-test"
    mode: str = "breathe"
    color_source: str = "temperature"
    fixed_color: str = "#2060ff"
    poll_interval_s: float = 2.0
    smoothing_tau_s: float = 3.0
    brightness_pct: float = 75.0
    speed: int = 235
    effect_brightness: int = 210
    write_leds: bool = True
    stops: tuple[ColorStop, ...] = field(default_factory=tuple)


def parse_config(raw: dict[str, Any]) -> Config:
    if not isinstance(raw, dict):
        raise ConfigError("config must be a JSON object")
    stops_raw = raw.get("stops", DEFAULT_STOPS)
    if not isinstance(stops_raw, (list, tuple)) or not stops_raw:
        raise ConfigError("stops must be a non-empty list")
    try:
        stops = tuple(ColorStop(float(item["temp_c"]), hex_to_rgb(item["color"])) for item in stops_raw)
    except (KeyError, TypeError, ValueError) as err:
        raise ConfigError(f"invalid colour stop: {err}") from err
    poll = float(raw.get("poll_interval_s", 2.0))
    if not 0.5 <= poll <= 60:
        raise ConfigError("poll_interval_s must be between 0.5 and 60")
    smoothing = float(raw.get("smoothing_tau_s", 3.0))
    if not 0 <= smoothing <= 60:
        raise ConfigError("smoothing_tau_s must be between 0 and 60")
    brightness = float(raw.get("brightness_pct", 75.0))
    if not 0 <= brightness <= 100:
        raise ConfigError("brightness_pct must be between 0 and 100")
    mode = str(raw.get("mode", "breathe"))
    if mode not in {"static", "breathe", "fade", "wink", "spectrum", "random"}:
        raise ConfigError("mode must be static, breathe, fade, wink, spectrum or random")
    color_source = str(raw.get("color_source", "temperature"))
    if color_source not in {"temperature", "fixed", "random"}:
        raise ConfigError("color_source must be temperature, fixed or random")
    fixed_color = str(raw.get("fixed_color", "#2060ff"))
    hex_to_rgb(fixed_color)
    speed = int(raw.get("speed", 235))
    effect_brightness = int(raw.get("effect_brightness", 210))
    if not 0 <= speed <= 255:
        raise ConfigError("speed must be between 0 and 255")
    if not 0 <= effect_brightness <= 255:
        raise ConfigError("effect_brightness must be between 0 and 255")
    return Config(
        coolercontrol_url=str(raw.get("coolercontrol_url", Config.coolercontrol_url)).rstrip("/"),
        coolercontrol_token=str(raw.get("coolercontrol_token", "")),
        temp_device_uid=str(raw.get("temp_device_uid", "")),
        gpu_name=str(raw.get("gpu_name", "NVIDIA GeForce RTX 5080")),
        temp_name=str(raw.get("temp_name", "GPU Temp")),
        driver_version=str(raw.get("driver_version", "")),
        controller=str(raw.get("controller", "spectra-write-test")),
        mode=mode,
        color_source=color_source,
        fixed_color=fixed_color,
        poll_interval_s=poll,
        smoothing_tau_s=smoothing,
        brightness_pct=brightness,
        speed=speed,
        effect_brightness=effect_brightness,
        write_leds=bool(raw.get("write_leds", True)),
        stops=stops,
    )


def load_config(path: Path) -> Config:
    if not path.exists():
        return parse_config({})
    try:
        return parse_config(json.loads(path.read_text(encoding="utf-8")))
    except json.JSONDecodeError as err:
        raise ConfigError(f"{path}: {err}") from err
