from gpu_led.colors import color_at, hex_to_rgb, scale_rgb
from gpu_led.config import ConfigError, parse_config
from gpu_led.coolercontrol import CoolerControlClient, CoolerControlError


def test_color_gradient() -> None:
    cfg = parse_config(
        {"stops": [{"temp_c": 40, "color": "#0000ff"}, {"temp_c": 60, "color": "#ff0000"}]}
    )
    assert color_at(40, cfg.stops) == (0, 0, 255)
    assert color_at(50, cfg.stops) == (186, 0, 194)
    assert color_at(70, cfg.stops) == (255, 0, 0)


def test_defaults_enable_continuous_led_updates() -> None:
    cfg = parse_config({})
    assert cfg.write_leds is True
    assert cfg.mode == "breathe"
    assert cfg.gpu_name == "NVIDIA GeForce RTX 5080"
    assert cfg.temp_name == "GPU Temp"
    assert cfg.smoothing_tau_s == 3.0
    assert cfg.brightness_pct == 75.0


def test_brightness_limits_rgb_command_level() -> None:
    assert scale_rgb((100, 200, 255), 75) == (75, 150, 191)


def test_write_configuration_requires_valid_mode() -> None:
    try:
        parse_config({"mode": "rainbow"})
    except ConfigError:
        pass
    else:
        raise AssertionError("invalid mode accepted")


def test_brightness_must_be_percent() -> None:
    for value in (-1, 101):
        try:
            parse_config({"brightness_pct": value})
        except ConfigError:
            continue
        raise AssertionError("invalid brightness accepted")


def test_hex_parser() -> None:
    assert hex_to_rgb("#12aBc0") == (0x12, 0xAB, 0xC0)


def test_gpu_selection_ignores_amd_gpu() -> None:
    client = CoolerControlClient("http://unused", "")
    client.status = lambda: {  # type: ignore[method-assign]
        "devices": [
            {"type": "GPU", "name": "AMD Radeon Graphics", "uid": "amd"},
            {"type": "GPU", "name": "NVIDIA GeForce RTX 5080", "uid": "nvidia"},
        ]
    }
    assert client.find_gpu_uid("NVIDIA GeForce RTX 5080", "GPU Temp") == "nvidia"


def test_gpu_selection_fails_when_nvidia_is_missing() -> None:
    client = CoolerControlClient("http://unused", "")
    client.status = lambda: {  # type: ignore[method-assign]
        "devices": [{"type": "GPU", "name": "AMD Radeon Graphics", "uid": "amd"}]
    }
    try:
        client.find_gpu_uid("NVIDIA GeForce RTX 5080", "GPU Temp")
    except CoolerControlError as err:
        assert "AMD Radeon Graphics" in str(err)
    else:
        raise AssertionError("missing NVIDIA GPU was accepted")
