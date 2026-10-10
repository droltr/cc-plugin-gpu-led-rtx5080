#!/usr/bin/env bash
set -euo pipefail

src=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
target=/var/lib/coolercontrol/plugins/gpu-led-rtx5080
stage="$target.new"
owner_home=$(getent passwd "${SUDO_USER:-root}" | cut -d: -f6)
include_ui=${INSTALL_UI:-0}
backup=""

case "$include_ui" in
    0|1) ;;
    *) echo "INSTALL_UI must be 0 or 1" >&2; exit 1 ;;
esac

test "$(id -u)" -eq 0 || { echo "run with sudo" >&2; exit 1; }
controller_bin=${SPECTRA_CONTROLLER_BIN:-}
config_driver_version=$(CONFIG_PATH="$src/config.json" python3 -c 'import json, os; print(json.load(open(os.environ["CONFIG_PATH"], encoding="utf-8"))["driver_version"])')
if [ -z "$controller_bin" ]; then
    test -n "${SPECTRA_NVIDIA_SRC:-}" || {
        echo "set SPECTRA_NVIDIA_SRC to the matching open-gpu-kernel-modules checkout" >&2
        exit 1
    }
    test -n "${SPECTRA_NVIDIA_COMMIT:-}" || {
        echo "set SPECTRA_NVIDIA_COMMIT to the verified NVIDIA source commit" >&2
        exit 1
    }
    actual_commit=$(git -C "$SPECTRA_NVIDIA_SRC" rev-parse HEAD 2>/dev/null) || {
        echo "SPECTRA_NVIDIA_SRC is not a Git checkout" >&2
        exit 1
    }
    test "$actual_commit" = "$SPECTRA_NVIDIA_COMMIT" || {
        echo "NVIDIA source commit mismatch: expected $SPECTRA_NVIDIA_COMMIT, got $actual_commit" >&2
        exit 1
    }
    git -C "$SPECTRA_NVIDIA_SRC" tag --points-at HEAD | grep -Fxq "$config_driver_version" || {
        echo "NVIDIA source checkout is not tagged $config_driver_version" >&2
        exit 1
    }
else
    test -n "${SPECTRA_CONTROLLER_SHA256:-}" || {
        echo "set SPECTRA_CONTROLLER_SHA256 for binary deployment" >&2
        exit 1
    }
    test -n "${SPECTRA_CONTROLLER_SOURCE_COMMIT:-}" || {
        echo "set SPECTRA_CONTROLLER_SOURCE_COMMIT for binary deployment" >&2
        exit 1
    }
    [[ "$SPECTRA_CONTROLLER_SOURCE_COMMIT" =~ ^[0-9a-fA-F]{7,40}$ ]] || {
        echo "SPECTRA_CONTROLLER_SOURCE_COMMIT must be a Git commit hash" >&2
        exit 1
    }
    test "${SPECTRA_CONTROLLER_DRIVER_VERSION:-}" = "$config_driver_version" || {
        echo "controller driver version does not match config.json" >&2
        exit 1
    }
    [[ "$SPECTRA_CONTROLLER_SHA256" =~ ^[0-9a-fA-F]{64}$ ]] || {
        echo "SPECTRA_CONTROLLER_SHA256 must be a SHA-256 digest" >&2
        exit 1
    }
    actual_hash=$(sha256sum "$controller_bin" | awk '{print $1}')
    test "$actual_hash" = "$SPECTRA_CONTROLLER_SHA256" || {
        echo "controller binary SHA-256 mismatch" >&2
        exit 1
    }
    build_info=$($controller_bin --build-info) || {
        echo "controller binary does not provide build metadata" >&2
        exit 1
    }
    actual_source_commit=$(printf '%s\n' "$build_info" | sed -n 's/^source_commit=//p')
    actual_driver_version=$(printf '%s\n' "$build_info" | sed -n 's/^driver_version=//p')
    test "$actual_source_commit" = "$SPECTRA_CONTROLLER_SOURCE_COMMIT" || {
        echo "controller source commit metadata mismatch" >&2
        exit 1
    }
    test "$actual_driver_version" = "$SPECTRA_CONTROLLER_DRIVER_VERSION" || {
        echo "controller driver metadata mismatch" >&2
        exit 1
    }
fi

rollback() {
    rm -rf "$stage"
    if [ -n "$backup" ] && [ -e "$backup" ]; then
        rm -rf "$target"
        mv "$backup" "$target"
    fi
}
trap rollback ERR

rm -rf "$stage"
mkdir -p "$stage/lib"
cp "$src/manifest.toml" "$src/run.sh" "$src/requirements.txt" "$stage/"
cp "$src/config.json" "$stage/config.json"
cp -r "$src/gpu_led" "$stage/lib/"
if [ "$include_ui" -eq 1 ]; then
    mkdir -p "$stage/ui"
    cp "$src/ui/index.html" "$stage/ui/index.html"
fi
if [ -n "$controller_bin" ]; then
    test -x "$controller_bin" || { echo "SPECTRA_CONTROLLER_BIN is not executable" >&2; exit 1; }
    cp "$controller_bin" "$stage/spectra-controller"
else
    make -C "$src/tools/spectra-write-test" clean spectra-controller \
        NVIDIA_SRC="$SPECTRA_NVIDIA_SRC" \
        SPECTRA_SOURCE_COMMIT="$actual_commit" \
        SPECTRA_DRIVER_VERSION="$config_driver_version"
    cp "$src/tools/spectra-write-test/spectra-controller" "$stage/"
fi
find "$stage/lib" -name __pycache__ -prune -exec rm -rf {} +

source_commit=${SPECTRA_CONTROLLER_SOURCE_COMMIT:-${SPECTRA_NVIDIA_COMMIT:-unavailable}}
cat > "$stage/controller-build.txt" <<EOF
driver_version=$config_driver_version
source_commit=$source_commit
sha256=$(sha256sum "$stage/spectra-controller" | awk '{print $1}')
EOF

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

if [ -e "$target" ]; then
    backup="$target.backup.$(date -u +%Y%m%dT%H%M%SZ).$$"
    mv "$target" "$backup"
fi
mv "$stage" "$target"
if command -v selinuxenabled >/dev/null && selinuxenabled; then
    chcon -t bin_t "$target/run.sh" "$target/spectra-controller"
fi
trap - ERR
echo "Installed to $target"
if [ -n "$backup" ]; then
    echo "Previous deployment retained at: $backup"
fi
echo "Restart with: sudo systemctl restart coolercontrold"
