#!/usr/bin/env bash
set -euo pipefail

hub_executable="${1:?Service Hub executable path is required}"
project_manager_executable="${2:?Project Manager executable path is required}"
client_executable="${3:?integration client executable path is required}"

temp_dir="$(mktemp -d)"
database_path="${temp_dir}/projects.db"
hub_log="${temp_dir}/hub.log"
project_log="${temp_dir}/project-manager.log"
hub_pid=""
project_pid=""
port=""

cleanup() {
    if [[ -n "${project_pid}" ]] && kill -0 "${project_pid}" 2>/dev/null; then
        kill -TERM "${project_pid}" 2>/dev/null || true
        wait "${project_pid}" 2>/dev/null || true
    fi
    if [[ -n "${hub_pid}" ]] && kill -0 "${hub_pid}" 2>/dev/null; then
        kill -TERM "${hub_pid}" 2>/dev/null || true
        wait "${hub_pid}" 2>/dev/null || true
    fi
    rm -rf "${temp_dir}"
}
trap cleanup EXIT

wait_for_hub() {
    local expected_port="${1:-}"
    for _ in $(seq 1 200); do
        if grep -q "Dispatcher Service Hub listening on" "${hub_log}"; then
            if [[ -z "${expected_port}" ]]; then
                port="$(sed -n 's/.*(bound port \([0-9][0-9]*\)).*/\1/p' "${hub_log}" | tail -n 1)"
            else
                port="${expected_port}"
            fi
            [[ -n "${port}" ]] && return 0
        fi
        if ! kill -0 "${hub_pid}" 2>/dev/null; then
            cat "${hub_log}"
            echo "Service Hub exited before becoming ready" >&2
            return 1
        fi
        sleep 0.05
    done
    cat "${hub_log}"
    echo "Service Hub did not become ready" >&2
    return 1
}

: >"${hub_log}"
"${hub_executable}" 127.0.0.1:0 >"${hub_log}" 2>&1 &
hub_pid="$!"
wait_for_hub

"${project_manager_executable}" "${database_path}" "127.0.0.1:${port}" >"${project_log}" 2>&1 &
project_pid="$!"

project_started=0
for _ in $(seq 1 200); do
    if grep -q "Dispatcher Project Manager started" "${project_log}"; then
        project_started=1
        break
    fi
    if ! kill -0 "${project_pid}" 2>/dev/null; then
        cat "${project_log}"
        echo "Project Manager exited before becoming ready" >&2
        exit 1
    fi
    sleep 0.05
done

if [[ "${project_started}" -ne 1 ]]; then
    cat "${project_log}"
    echo "Project Manager did not become ready" >&2
    exit 1
fi

"${client_executable}" 127.0.0.1 "${port}" initial

kill -TERM "${hub_pid}"
wait "${hub_pid}"
hub_pid=""

: >"${hub_log}"
"${hub_executable}" "127.0.0.1:${port}" >"${hub_log}" 2>&1 &
hub_pid="$!"
wait_for_hub "${port}"

"${client_executable}" 127.0.0.1 "${port}" after-reconnect

kill -TERM "${project_pid}"
wait "${project_pid}"
project_pid=""

if ! grep -q "Dispatcher Project Manager stopped" "${project_log}"; then
    cat "${project_log}"
    echo "Project Manager did not stop cleanly" >&2
    exit 1
fi
