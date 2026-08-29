#!/usr/bin/env bash
set -euo pipefail

executable="${1:?Users & Access executable path is required}"
signal_name="${2:?signal name is required}"
signal_label="${3:?signal label is required}"

log_file="$(mktemp)"
pid=""

cleanup() {
    if [[ -n "${pid}" ]] && kill -0 "${pid}" 2>/dev/null; then
        kill -KILL "${pid}" 2>/dev/null || true
        wait "${pid}" 2>/dev/null || true
    fi
    rm -f "${log_file}"
}
trap cleanup EXIT

"${executable}" >"${log_file}" 2>&1 &
pid="$!"

started=0
for _ in $(seq 1 100); do
    if grep -q "Dispatcher Users & Access started" "${log_file}"; then
        started=1
        break
    fi
    if ! kill -0 "${pid}" 2>/dev/null; then
        cat "${log_file}"
        echo "Users & Access exited before becoming ready" >&2
        exit 1
    fi
    sleep 0.05
done

if [[ "${started}" -ne 1 ]]; then
    cat "${log_file}"
    echo "Users & Access did not report readiness" >&2
    exit 1
fi

kill -s "${signal_name}" "${pid}"

set +e
wait "${pid}"
status="$?"
set -e
pid=""

if [[ "${status}" -ne 0 ]]; then
    cat "${log_file}"
    echo "Users & Access exited with status ${status}" >&2
    exit 1
fi

if ! grep -q "shutdown requested by ${signal_label}" "${log_file}"; then
    cat "${log_file}"
    echo "Users & Access did not report ${signal_label}" >&2
    exit 1
fi

if ! grep -q "Dispatcher Users & Access stopped" "${log_file}"; then
    cat "${log_file}"
    echo "Users & Access did not report a clean stop" >&2
    exit 1
fi
