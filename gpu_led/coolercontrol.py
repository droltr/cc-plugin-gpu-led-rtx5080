"""Minimal read-only CoolerControl REST client."""

import json
from typing import Any
from urllib.request import Request, urlopen


class CoolerControlError(RuntimeError):
    pass


class CoolerControlClient:
    def __init__(self, base_url: str, token: str, timeout_s: float = 3.0) -> None:
        self.base_url = base_url.rstrip("/")
        self.token = token
        self.timeout_s = timeout_s

    def status(self) -> dict[str, Any]:
        request = Request(f"{self.base_url}/status")
        if self.token:
            request.add_header("Authorization", f"Bearer {self.token}")
        try:
            with urlopen(request, timeout=self.timeout_s) as response:
                data = json.load(response)
        except Exception as err:
            raise CoolerControlError(f"CoolerControl status failed: {err}") from err
        if not isinstance(data, dict) or not isinstance(data.get("devices"), list):
            raise CoolerControlError("unexpected CoolerControl /status response")
        return data

    def find_gpu_uid(self, model: str, temp_name: str) -> str:
        devices = self.status()["devices"]
        wanted = model.casefold()
        matches = []
        for device in devices:
            if device.get("type") != "GPU":
                continue
            names = (device.get("name"), device.get("label"), device.get("description"))
            name_match = any(wanted in str(name).casefold() for name in names if name)
            history = device.get("status_history") or []
            temps = history[-1].get("temps", []) if history else []
            sensor_match = any(sample.get("name") == temp_name for sample in temps)
            if name_match or (not any(names) and sensor_match):
                matches.append(str(device["uid"]))
        if len(matches) == 1:
            return matches[0]
        if not matches:
            available = [str(device.get("name") or device.get("uid", "unknown"))
                         for device in devices if device.get("type") == "GPU"]
            raise CoolerControlError(f"GPU {model!r} not found; available GPUs: {available}")
        raise CoolerControlError(f"GPU model {model!r} matched more than one device")

    def read_temp(self, uid: str, name: str) -> float:
        for device in self.status()["devices"]:
            if device.get("uid") != uid:
                continue
            history = device.get("status_history") or []
            if history:
                for sample in history[-1].get("temps", []):
                    if sample.get("name") == name:
                        return float(sample["temp"])
        raise CoolerControlError(f"temperature {name!r} not found for GPU {uid[:8]}")
