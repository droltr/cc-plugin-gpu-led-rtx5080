"""Subprocess boundary to the C NVIDIA RM controller."""

import subprocess
from pathlib import Path

from .colors import RGB


class ControllerError(RuntimeError):
    pass


class SpectraController:
    def __init__(self, executable: Path, driver_version: str, mode: str,
                 speed: int = 235, effect_brightness: int = 210) -> None:
        if not driver_version:
            raise ControllerError("driver_version is required before LED writes are enabled")
        self.executable = executable
        self.driver_version = driver_version
        self.mode = mode
        self.speed = speed
        self.effect_brightness = effect_brightness

    def set_color(self, color: RGB) -> None:
        command = [
            str(self.executable), self.driver_version, "set", self.mode,
            *(str(channel) for channel in color), str(self.speed), str(self.effect_brightness),
        ]
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=8, check=False)
        except (OSError, subprocess.TimeoutExpired) as err:
            raise ControllerError(f"GPU LED controller failed: {err}") from err
        if result.returncode:
            detail = (result.stderr or result.stdout).strip()[-500:]
            raise ControllerError(f"GPU LED controller exited {result.returncode}: {detail}")
