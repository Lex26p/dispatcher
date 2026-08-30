#!/usr/bin/env bash
set -euo pipefail

hub_executable="${1:?Service Hub executable path is required}"
users_access_executable="${2:?Users & Access executable path is required}"
client_executable="${3:?integration client executable path is required}"

temp_dir="$(mktemp -d)"
database_path="${temp_dir}/users-access.db"
hub_log="${temp_dir}/hub.log"
users_access_log="${temp_dir}/users-access.log"
bootstrap_log="${temp_dir}/bootstrap.log"
hub_pid=""
users_access_pid=""
port=""
bootstrap_password="integration password 123"

cleanup() {
    if [[ -n "${users_access_pid}" ]] && kill -0 "${users_access_pid}" 2>/dev/null; then
        kill -TERM "${users_access_pid}" 2>/dev/null || true
        wait "${users_access_pid}" 2>/dev/null || true
    fi
    if [[ -n "${hub_pid}" ]] && kill -0 "${hub_pid}" 2>/dev/null; then
        kill -TERM "${hub_pid}" 2>/dev/null || true
        wait "${hub_pid}" 2>/dev/null || true
    fi
    rm -rf "${temp_dir}"
}
trap cleanup EXIT

printf '%s\n%s\n' "${bootstrap_password}" "${bootstrap_password}" |
    "${users_access_executable}" \
        --bootstrap-admin \
        integration-admin \
        "Integration Administrator" \
        "${database_path}" >"${bootstrap_log}" 2>&1

if grep -Fq "${bootstrap_password}" "${bootstrap_log}"; then
    cat "${bootstrap_log}"
    echo "Bootstrap diagnostics leaked the test password" >&2
    exit 1
fi

"${hub_executable}" 127.0.0.1:0 >"${hub_log}" 2>&1 &
hub_pid="$!"

for _ in $(seq 1 200); do
    if grep -q "Dispatcher Service Hub listening on" "${hub_log}"; then
        port="$(sed -n 's/.*(bound port \([0-9][0-9]*\)).*/\1/p' "${hub_log}" | tail -n 1)"
        [[ -n "${port}" ]] && break
    fi
    if ! kill -0 "${hub_pid}" 2>/dev/null; then
        cat "${hub_log}"
        echo "Service Hub exited before becoming ready" >&2
        exit 1
    fi
    sleep 0.05
done

if [[ -z "${port}" ]]; then
    cat "${hub_log}"
    echo "Service Hub did not become ready" >&2
    exit 1
fi

"${users_access_executable}" \
    "${database_path}" \
    "127.0.0.1:${port}" >"${users_access_log}" 2>&1 &
users_access_pid="$!"

started=0
for _ in $(seq 1 200); do
    if grep -q "Dispatcher Users & Access started" "${users_access_log}"; then
        started=1
        break
    fi
    if ! kill -0 "${users_access_pid}" 2>/dev/null; then
        cat "${users_access_log}"
        echo "Users & Access exited before becoming ready" >&2
        exit 1
    fi
    sleep 0.05
done

if [[ "${started}" -ne 1 ]]; then
    cat "${users_access_log}"
    echo "Users & Access did not become ready" >&2
    exit 1
fi

"${client_executable}" 127.0.0.1 "${port}"

kill -TERM "${users_access_pid}"
wait "${users_access_pid}"
users_access_pid=""

if ! grep -q "Dispatcher Users & Access stopped" "${users_access_log}"; then
    cat "${users_access_log}"
    echo "Users & Access did not stop cleanly" >&2
    exit 1
fi
