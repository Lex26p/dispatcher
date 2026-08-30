#!/usr/bin/env bash
set -euo pipefail

hub_executable="${1:?Service Hub executable path is required}"
project_manager_executable="${2:?Project Manager executable path is required}"
client_executable="${3:?integration client executable path is required}"

project_manager_dir="$(dirname "${project_manager_executable}")"
services_build_dir="$(dirname "${project_manager_dir}")"
users_access_executable="${services_build_dir}/users-access/dispatcher-users-access"
fixture_executable="${services_build_dir}/users-access/dispatcher-users-access-project-manager-authorization-fixture"

if [[ ! -x "${users_access_executable}" ]]; then
    echo "Users & Access executable is missing: ${users_access_executable}" >&2
    exit 1
fi
if [[ ! -x "${fixture_executable}" ]]; then
    echo "Users & Access authorization fixture is missing: ${fixture_executable}" >&2
    exit 1
fi

temp_dir="$(mktemp -d)"
project_database="${temp_dir}/projects.db"
users_database="${temp_dir}/users-access.db"
projects_file="${temp_dir}/projects.txt"
operator_token_file="${temp_dir}/operator.token"
expiring_token_file="${temp_dir}/expiring.token"
operator_password_file="${temp_dir}/operator.password"
expiring_password_file="${temp_dir}/expiring.password"
project_admin_password_file="${temp_dir}/project-admin.password"

hub_log="${temp_dir}/hub.log"
project_log="${temp_dir}/project-manager.log"
users_log="${temp_dir}/users-access.log"
bootstrap_log="${temp_dir}/bootstrap.log"

hub_pid=""
project_pid=""
users_pid=""
port=""

admin_password='Step5 integration admin password'
operator_password='Step5 integration operator password'
expiring_password='Step5 integration expiring password'
project_admin_password='Step5 integration project admin password'

printf '%s\n' "${operator_password}" >"${operator_password_file}"
printf '%s\n' "${expiring_password}" >"${expiring_password_file}"
printf '%s\n' "${project_admin_password}" >"${project_admin_password_file}"
chmod 600     "${operator_password_file}"     "${expiring_password_file}"     "${project_admin_password_file}"

cleanup() {
    if [[ -n "${project_pid}" ]] && kill -0 "${project_pid}" 2>/dev/null; then
        kill -TERM "${project_pid}" 2>/dev/null || true
        wait "${project_pid}" 2>/dev/null || true
    fi
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

start_hub() {
    local address="${1:-127.0.0.1:0}"
    : >"${hub_log}"
    "${hub_executable}" "${address}" >"${hub_log}" 2>&1 &
    hub_pid="$!"
    wait_for_log \
        "${hub_pid}" \
        "${hub_log}" \
        "Dispatcher Service Hub listening on" \
        "Service Hub"

    if [[ "${address}" == "127.0.0.1:0" ]]; then
        port="$(
            sed -n 's/.*(bound port \([0-9][0-9]*\)).*/\1/p' \
                "${hub_log}" | tail -n 1
        )"
        if [[ -z "${port}" ]]; then
            cat "${hub_log}" >&2
            echo "Could not resolve Service Hub bound port" >&2
            exit 1
        fi
    fi
}

stop_hub() {
    if [[ -n "${hub_pid}" ]] && kill -0 "${hub_pid}" 2>/dev/null; then
        kill -TERM "${hub_pid}"
        wait "${hub_pid}"
    fi
    hub_pid=""
}

start_users_access() {
    : >"${users_log}"
    "${users_access_executable}" \
        "${users_database}" \
        "127.0.0.1:${port}" \
        >"${users_log}" 2>&1 &
    users_pid="$!"
    wait_for_log \
        "${users_pid}" \
        "${users_log}" \
        "Dispatcher Users & Access started" \
        "Users & Access"
}

stop_users_access() {
    if [[ -n "${users_pid}" ]] && kill -0 "${users_pid}" 2>/dev/null; then
        kill -TERM "${users_pid}"
        wait "${users_pid}"
    fi
    users_pid=""
}

start_project_manager() {
    : >"${project_log}"
    "${project_manager_executable}" \
        "${project_database}" \
        "127.0.0.1:${port}" \
        >"${project_log}" 2>&1 &
    project_pid="$!"
    wait_for_log \
        "${project_pid}" \
        "${project_log}" \
        "Dispatcher Project Manager started" \
        "Project Manager"
}

printf '%s\n%s\n' "${admin_password}" "${admin_password}" |
    "${users_access_executable}" \
        --bootstrap-admin \
        step5-admin \
        "Step 5 Admin" \
        "${users_database}" \
        >"${bootstrap_log}" 2>&1

if grep -q "${admin_password}" "${bootstrap_log}"; then
    cat "${bootstrap_log}" >&2
    echo "Bootstrap output leaked test password" >&2
    exit 1
fi

start_hub
start_users_access
start_project_manager

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    admin-setup \
    "${projects_file}"

visible_project_id="$(sed -n '1p' "${projects_file}")"
if [[ -z "${visible_project_id}" ]]; then
    echo "Visible project id is missing" >&2
    exit 1
fi

stop_users_access

"${fixture_executable}" \
    create-user \
    "${users_database}" \
    step5-operator \
    "Step 5 Operator" \
    "${operator_password_file}" \
    "${visible_project_id}" \
    editor

"${fixture_executable}" \
    create-user \
    "${users_database}" \
    step5-expiring \
    "Step 5 Expiring User" \
    "${expiring_password_file}" \
    "${visible_project_id}" \
    editor

hidden_project_id="$(sed -n '2p' "${projects_file}")"
if [[ -z "${hidden_project_id}" ]]; then
    echo "Hidden project id is missing" >&2
    exit 1
fi

"${fixture_executable}" \
    create-user \
    "${users_database}" \
    step5-project-admin \
    "Step 5 Project Admin" \
    "${project_admin_password_file}" \
    "${hidden_project_id}" \
    admin

start_users_access

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    restricted \
    "${projects_file}" \
    "${operator_token_file}" \
    "${expiring_token_file}"

stop_users_access

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    unavailable \
    "${projects_file}" \
    "${operator_token_file}"

start_users_access

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    recovered \
    "${projects_file}" \
    "${operator_token_file}"

stop_users_access

"${fixture_executable}" \
    revoke-project \
    "${users_database}" \
    step5-operator \
    "${visible_project_id}"

start_users_access

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    revoked \
    "${projects_file}" \
    "${operator_token_file}"

stop_users_access

"${fixture_executable}" \
    set-enabled \
    "${users_database}" \
    step5-operator \
    0

"${fixture_executable}" \
    expire-session \
    "${users_database}" \
    "${expiring_token_file}"

start_users_access

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    disabled \
    "${projects_file}" \
    "${operator_token_file}"

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    expired \
    "${projects_file}" \
    "${expiring_token_file}"

stop_hub

start_hub "127.0.0.1:${port}"

"${client_executable}" \
    127.0.0.1 \
    "${port}" \
    after-hub-reconnect \
    "${projects_file}"

kill -TERM "${project_pid}"
wait "${project_pid}"
project_pid=""

stop_users_access
stop_hub

if ! grep -q "Dispatcher Project Manager stopped" "${project_log}"; then
    cat "${project_log}" >&2
    echo "Project Manager did not stop cleanly" >&2
    exit 1
fi

if ! grep -q "Dispatcher Users & Access stopped" "${users_log}"; then
    cat "${users_log}" >&2
    echo "Users & Access did not stop cleanly" >&2
    exit 1
fi

if grep -q "${operator_password}" "${project_log}" "${users_log}" 2>/dev/null ||
   grep -q "${expiring_password}" "${project_log}" "${users_log}" 2>/dev/null ||
   grep -q "${project_admin_password}" "${project_log}" "${users_log}" 2>/dev/null; then
    echo "Service logs leaked test credential material" >&2
    exit 1
fi
