#!/usr/bin/env bash
set -euo pipefail

executable="${1:?Users & Access executable path is required}"

temp_dir="$(mktemp -d)"
database_path="${temp_dir}/users-access.db"
log_file="${temp_dir}/bootstrap.log"
secret="test-only-bootstrap-password-12345"

cleanup() {
    rm -rf "${temp_dir}"
}
trap cleanup EXIT

printf '%s\n%s\n' "${secret}" "${secret}" |
    "${executable}" --bootstrap-admin admin "Bootstrap Administrator" "${database_path}" \
    >"${log_file}" 2>&1

if ! grep -q "first administrator created" "${log_file}"; then
    cat "${log_file}"
    echo "Bootstrap did not report success" >&2
    exit 1
fi

if [[ ! -f "${database_path}" ]]; then
    cat "${log_file}"
    echo "Bootstrap did not create SQLite database" >&2
    exit 1
fi

if grep -Fq "${secret}" "${log_file}"; then
    echo "Bootstrap secret leaked to process output" >&2
    exit 1
fi

if grep -aFq "${secret}" "${database_path}"; then
    echo "Bootstrap secret leaked to SQLite storage" >&2
    exit 1
fi

set +e
printf '%s\n%s\n' "another-test-only-password-12345" "another-test-only-password-12345" |
    "${executable}" --bootstrap-admin second "Second Administrator" "${database_path}" \
    >"${log_file}" 2>&1
status="$?"
set -e

if [[ "${status}" -eq 0 ]]; then
    cat "${log_file}"
    echo "Second bootstrap unexpectedly succeeded" >&2
    exit 1
fi

if ! grep -q "already initialized" "${log_file}"; then
    cat "${log_file}"
    echo "Second bootstrap did not report initialized storage" >&2
    exit 1
fi
