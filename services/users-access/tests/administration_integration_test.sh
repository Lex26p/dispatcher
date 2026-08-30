#!/usr/bin/env bash
set -euo pipefail

hub_executable="${1:?Service Hub executable path is required}"
users_access_executable="${2:?Users & Access executable path is required}"
client_executable="${3:?administration integration client path is required}"

temp_dir="$(mktemp -d)"
database="${temp_dir}/users-access.db"
hub_log="${temp_dir}/hub.log"
users_log="${temp_dir}/users.log"
bootstrap_log="${temp_dir}/bootstrap.log"
client_log="${temp_dir}/client.log"

hub_pid=""
users_pid=""
admin_password='Step6A integration administrator password'

cleanup() {
    if [[ -n "${users_pid}" ]] && kill -0 "${users_pid}" 2>/dev/null; then
        kill -TERM "${users_pid}" 2>/dev/null || true
        wait "${users_pid}" 2>/dev/null || true
    fi
    if [[ -n "${hub_pid}" ]] && kill -0 "${hub_pid}" 2>/dev/null; then
        kill -TERM "${hub_pid}" 2>/dev/null || true
        wait "${hub_pid}" 2>/dev/null || true
    fi
    rm -rf "${temp_dir}"
}
trap cleanup EXIT

wait_for_log() {
    local pid="${1:?pid required}"
    local log="${2:?log required}"
    local pattern="${3:?pattern required}"
    local label="${4:?label required}"

    for _ in $(seq 1 300); do
        if grep -q "${pattern}" "${log}" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "${pid}" 2>/dev/null; then
            cat "${log}" >&2 || true
            echo "${label} exited before becoming ready" >&2
            return 1
        fi
        sleep 0.05
    done

    cat "${log}" >&2 || true
    echo "${label} did not become ready" >&2
    return 1
}

printf '%s\n%s\n' "${admin_password}" "${admin_password}" |
    "${users_access_executable}" \
        --bootstrap-admin \
        step6a-admin \
        "Step 6A Administrator" \
        "${database}" \
        >"${bootstrap_log}" 2>&1

if grep -q "${admin_password}" "${bootstrap_log}"; then
    echo "bootstrap output leaked administrator password" >&2
    exit 1
fi

"${hub_executable}" "127.0.0.1:0" >"${hub_log}" 2>&1 &
hub_pid="$!"
wait_for_log "${hub_pid}" "${hub_log}" "Dispatcher Service Hub listening on" "Service Hub"

port="$(sed -n 's/.*(bound port \([0-9][0-9]*\)).*/\1/p' "${hub_log}" | tail -n 1)"
if [[ -z "${port}" ]]; then
    cat "${hub_log}" >&2
    echo "could not resolve Service Hub bound port" >&2
    exit 1
fi

"${users_access_executable}" \
    "${database}" \
    "127.0.0.1:${port}" \
    >"${users_log}" 2>&1 &
users_pid="$!"
wait_for_log "${users_pid}" "${users_log}" "Dispatcher Users & Access started" "Users & Access"

printf '%s\n' "${admin_password}" |
    "${client_executable}" 127.0.0.1 "${port}" step6a-admin \
    >"${client_log}" 2>&1

if ! grep -q "Users & Access administration integration passed" "${client_log}"; then
    cat "${client_log}" >&2
    exit 1
fi

if grep -q "${admin_password}" "${hub_log}" "${users_log}" "${client_log}" 2>/dev/null; then
    echo "service/test output leaked administrator password" >&2
    exit 1
fi

kill -TERM "${users_pid}"
wait "${users_pid}"
users_pid=""

kill -TERM "${hub_pid}"
wait "${hub_pid}"
hub_pid=""

if ! grep -q "Dispatcher Users & Access stopped" "${users_log}"; then
    cat "${users_log}" >&2
    echo "Users & Access did not stop cleanly" >&2
    exit 1
fi
