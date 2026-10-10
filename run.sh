#!/usr/bin/env bash
set -euo pipefail
dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
export PYTHONPATH="$dir/lib"
exec "$dir/venv/bin/python" -P -m gpu_led --config "$dir/config.json" \
    --controller "$dir/spectra-controller" "$@"
