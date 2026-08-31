#!/usr/bin/env bash
set -euo pipefail

executable="${1:?Device Manager executable path is required}"
signal_name="${2:?signal name is required}"
expected_name="${3:?expected signal name is required}"

temp_dir="$(mktemp -d)"
database="${temp_dir}/device-manager.db"
log="${temp_dir}/device-manager.log"
pid=""

cleanup() {
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
        kill -TERM "${pid}" 2>/dev/null || true
        wait "${pid}" 2>/dev/null || true
    fi
    rm -rf "${temp_dir}"
}
trap cleanup EXIT

"${executable}" "${database}" >"${log}" 2>&1 &
pid="$!"

for _ in $(seq 1 200); do
    if grep -q "Dispatcher Device Manager started" "${log}" 2>/dev/null; then
        break
    fi
    if ! kill -0 "${pid}" 2>/dev/null; then
        cat "${log}" >&2 || true
        echo "Device Manager exited before becoming ready" >&2
        exit 1
    fi
    sleep 0.02
done

if ! grep -q "Dispatcher Device Manager SQLite storage ready" "${log}"; then
    cat "${log}" >&2 || true
    echo "Device Manager storage did not become ready" >&2
    exit 1
fi

if ! grep -q "Dispatcher Device Manager started" "${log}"; then
    cat "${log}" >&2 || true
    echo "Device Manager did not become ready" >&2
    exit 1
fi

kill "-${signal_name}" "${pid}"
wait "${pid}"
pid=""

if ! grep -q "shutdown requested by ${expected_name}" "${log}"; then
    cat "${log}" >&2
    echo "Device Manager did not report ${expected_name}" >&2
    exit 1
fi

if ! grep -q "Dispatcher Device Manager stopped" "${log}"; then
    cat "${log}" >&2
    echo "Device Manager did not stop cleanly" >&2
    exit 1
fi
