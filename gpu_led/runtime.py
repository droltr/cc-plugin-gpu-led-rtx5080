"""Temperature-driven LED runtime, kept separate from the settings interface."""

import logging
import math
import random
import threading
import time
from pathlib import Path

from .backend import SpectraController
from .colors import color_at, hex_to_rgb, scale_rgb
from .config import Config, ConfigError, load_config
from .coolercontrol import CoolerControlClient, CoolerControlError

LOG = logging.getLogger("gpu_led")


class Ema:
    """Exponential moving average used by the temperature-to-colour runtime."""

    def __init__(self, tau_s: float) -> None:
        self.tau_s = max(0.0, tau_s)
        self.value: float | None = None

    def update(self, sample: float, dt_s: float) -> float:
        if self.value is None or self.tau_s == 0:
            self.value = sample
        else:
            alpha = 1 - math.exp(-max(0.0, dt_s) / self.tau_s)
            self.value += alpha * (sample - self.value)
        return self.value


def color_changed(old: tuple[int, int, int] | None, new: tuple[int, int, int]) -> bool:
    return old is None or any(abs(a - b) > 2 for a, b in zip(old, new, strict=True))


def color_for_temperature(temp: float, cfg: Config) -> tuple[int, int, int]:
    if cfg.color_source == "temperature":
        return color_at(temp, cfg.stops)
    if cfg.color_source == "fixed":
        return hex_to_rgb(cfg.fixed_color)
    if cfg.mode in {"fade", "spectrum", "random"}:
        return (0, 0, 0)
    return tuple(random.randrange(256) for _ in range(3))


class ThermalLedRuntime:
    """Run the stable GPU-temperature-to-LED path using a validated config snapshot."""

    def __init__(self, config_path: Path, controller_path: Path) -> None:
        self.config_path = config_path
        self.controller_path = controller_path

    def run(self, stop: threading.Event) -> int:
        cfg = load_config(self.config_path)
        config_mtime = self.config_path.stat().st_mtime_ns if self.config_path.exists() else None
        cc = CoolerControlClient(cfg.coolercontrol_url, cfg.coolercontrol_token)
        uid = cfg.temp_device_uid or cc.find_gpu_uid(cfg.gpu_name, cfg.temp_name)
        controller = self._make_controller(cfg)
        last_color = None
        ema = Ema(cfg.smoothing_tau_s)
        last_tick = time.monotonic()
        while not stop.is_set():
            try:
                current_mtime = self.config_path.stat().st_mtime_ns if self.config_path.exists() else None
                if current_mtime != config_mtime:
                    try:
                        new_cfg = load_config(self.config_path)
                    except ConfigError as err:
                        LOG.error("ignoring invalid configuration change: %s", err)
                    else:
                        cfg = new_cfg
                        config_mtime = current_mtime
                        cc = CoolerControlClient(cfg.coolercontrol_url, cfg.coolercontrol_token)
                        uid = cfg.temp_device_uid or cc.find_gpu_uid(cfg.gpu_name, cfg.temp_name)
                        controller = self._make_controller(cfg)
                        last_color = None
                        LOG.info("configuration reloaded without restarting the plugin")
                now = time.monotonic()
                temp = ema.update(cc.read_temp(uid, cfg.temp_name), now - last_tick)
                last_tick = now
                color = scale_rgb(color_for_temperature(temp, cfg), cfg.brightness_pct)
                if color_changed(last_color, color):
                    LOG.info("GPU %.1f °C -> #%02x%02x%02x", temp, *color)
                    if controller:
                        controller.set_color(color)
                    last_color = color
            except (CoolerControlError, OSError, RuntimeError) as err:
                LOG.warning("GPU LED update failed: %s", err)
            stop.wait(cfg.poll_interval_s)
        return 0

    def _make_controller(self, cfg: Config) -> SpectraController | None:
        if not cfg.write_leds:
            return None
        return SpectraController(
            self.controller_path, cfg.driver_version, cfg.mode, cfg.speed, cfg.effect_brightness
        )


__all__ = ["ThermalLedRuntime"]
