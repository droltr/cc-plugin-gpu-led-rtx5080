import unittest

from gpu_led.colors import color_at, hex_to_rgb, scale_rgb
from gpu_led.config import ConfigError, parse_config
from gpu_led.coolercontrol import CoolerControlClient, CoolerControlError


class GpuLedTests(unittest.TestCase):
    def test_color_gradient(self) -> None:
        cfg = parse_config(
            {"stops": [{"temp_c": 40, "color": "#0000ff"}, {"temp_c": 60, "color": "#ff0000"}]}
        )
        self.assertEqual(color_at(40, cfg.stops), (0, 0, 255))
        self.assertEqual(color_at(50, cfg.stops), (186, 0, 194))
        self.assertEqual(color_at(70, cfg.stops), (255, 0, 0))

    def test_defaults_enable_continuous_led_updates(self) -> None:
        cfg = parse_config({})
        self.assertTrue(cfg.write_leds)
        self.assertEqual(cfg.mode, "breathe")
        self.assertEqual(cfg.gpu_name, "NVIDIA GeForce RTX 5080")
        self.assertEqual(cfg.temp_name, "GPU Temp")
        self.assertEqual(cfg.smoothing_tau_s, 3.0)
        self.assertEqual(cfg.brightness_pct, 75.0)

    def test_brightness_limits_rgb_command_level(self) -> None:
        self.assertEqual(scale_rgb((100, 200, 255), 75), (75, 150, 191))

    def test_write_configuration_requires_valid_mode(self) -> None:
        with self.assertRaises(ConfigError):
            parse_config({"mode": "rainbow"})

    def test_brightness_must_be_percent(self) -> None:
        for value in (-1, 101):
            with self.subTest(value=value), self.assertRaises(ConfigError):
                parse_config({"brightness_pct": value})

    def test_hex_parser(self) -> None:
        self.assertEqual(hex_to_rgb("#12aBc0"), (0x12, 0xAB, 0xC0))

    def test_gpu_selection_ignores_amd_gpu(self) -> None:
        client = CoolerControlClient("http://unused", "")
        client.status = lambda: {  # type: ignore[method-assign]
            "devices": [
                {"type": "GPU", "name": "AMD Radeon Graphics", "uid": "amd"},
                {"type": "GPU", "name": "NVIDIA GeForce RTX 5080", "uid": "nvidia"},
            ]
        }
        self.assertEqual(client.find_gpu_uid("NVIDIA GeForce RTX 5080", "GPU Temp"), "nvidia")

    def test_gpu_selection_fails_when_nvidia_is_missing(self) -> None:
        client = CoolerControlClient("http://unused", "")
        client.status = lambda: {  # type: ignore[method-assign]
            "devices": [{"type": "GPU", "name": "AMD Radeon Graphics", "uid": "amd"}]
        }
        with self.assertRaisesRegex(CoolerControlError, "AMD Radeon Graphics"):
            client.find_gpu_uid("NVIDIA GeForce RTX 5080", "GPU Temp")
