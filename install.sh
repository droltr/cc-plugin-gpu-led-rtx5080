#!/usr/bin/env bash
set -euo pipefail

src=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
target=/var/lib/coolercontrol/plugins/gpu-led-rtx5080
stage="$target.new"
owner_home=$(getent passwd "${SUDO_USER:-root}" | cut -d: -f6)
include_ui=${INSTALL_UI:-0}

case "$include_ui" in
    0|1) ;;
    *) echo "INSTALL_UI must be 0 or 1" >&2; exit 1 ;;
esac

test "$(id -u)" -eq 0 || { echo "run with sudo" >&2; exit 1; }
test -n "${SPECTRA_NVIDIA_SRC:-}" || {
    echo "set SPECTRA_NVIDIA_SRC to the matching open-gpu-kernel-modules checkout" >&2
    exit 1
}

rm -rf "$stage"
mkdir -p "$stage/lib"
cp "$src/manifest.toml" "$src/run.sh" "$src/requirements.txt" "$stage/"
cp "$src/config.json" "$stage/config.json"
cp -r "$src/gpu_led" "$stage/lib/"
if [ "$include_ui" -eq 1 ]; then
    mkdir -p "$stage/ui"
    cp "$src/ui/index.html" "$stage/ui/index.html"
fi
make -C "$src/tools/spectra-write-test" clean spectra-controller NVIDIA_SRC="$SPECTRA_NVIDIA_SRC"
cp "$src/tools/spectra-write-test/spectra-controller" "$stage/"
find "$stage/lib" -name __pycache__ -prune -exec rm -rf {} +

/usr/bin/python3 -m venv "$stage/venv"
"$stage/venv/bin/pip" install --quiet --disable-pip-version-check -r "$stage/requirements.txt"
chmod 0755 "$stage/run.sh" "$stage/spectra-controller"

token_file="$owner_home/.config/cc-token"
if [ -f "$token_file" ]; then
    TOKEN_FILE="$token_file" CONFIG_FILE="$stage/config.json" python3 - <<'PY'
import json
import os

config_path = os.environ["CONFIG_FILE"]
token_path = os.environ["TOKEN_FILE"]
with open(config_path, encoding="utf-8") as stream:
    config = json.load(stream)
with open(token_path, encoding="utf-8") as stream:
    config["coolercontrol_token"] = stream.read().strip()
with open(config_path, "w", encoding="utf-8") as stream:
    json.dump(config, stream, indent=2)
    stream.write("\n")
PY
fi
chmod 0600 "$stage/config.json"

rm -rf "$target"
mv "$stage" "$target"
if command -v selinuxenabled >/dev/null && selinuxenabled; then
    chcon -t bin_t "$target/run.sh" "$target/spectra-controller"
fi
echo "Installed to $target"
echo "Restart with: sudo systemctl restart coolercontrold"
