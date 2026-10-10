"""CoolerControl entry point for the stable GPU LED runtime."""

import argparse
import logging
import os
import signal
import threading
from pathlib import Path

from .config import ConfigError
from .runtime import ThermalLedRuntime


def main() -> int:
    parser = argparse.ArgumentParser(description="Temperature-driven ZOTAC GPU LED colour")
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--controller", type=Path, required=True)
    args = parser.parse_args()
    logging.basicConfig(
        level=os.environ.get("CC_LOG", "INFO"),
        format="%(levelname)s %(name)s: %(message)s",
    )
    stop = threading.Event()
    for sig in (signal.SIGTERM, signal.SIGINT):
        signal.signal(sig, lambda *_: stop.set())
    try:
        return ThermalLedRuntime(args.config, args.controller).run(stop)
    except ConfigError as err:
        logging.getLogger("gpu_led").error("Invalid configuration: %s", err)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
