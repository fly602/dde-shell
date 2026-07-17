#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

set -uo pipefail

readonly script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
readonly app="${script_dir}/appearance-opacity-example"
readonly stamp="$(date +%Y%m%d-%H%M%S)"
readonly output_dir="${script_dir}/opacity-logs-${stamp}"
readonly archive="${output_dir}.tar.gz"

if [[ ! -x "${app}" ]]; then
    printf '错误：找不到可执行程序 %s\n' "${app}" >&2
    exit 1
fi

mkdir -p "${output_dir}"

collect_command() {
    local command_name="$1"
    shift
    printf '\n===== %s %s =====\n' "${command_name}" "$*"
    if command -v "${command_name}" >/dev/null 2>&1; then
        "${command_name}" "$@" 2>&1 || true
    else
        printf '未安装 %s\n' "${command_name}"
    fi
}

count_pngs() (
    shopt -s nullglob
    local files=("$1"/*.png)
    printf '%d' "${#files[@]}"
)

{
    printf '采集时间：%s\n' "$(date --iso-8601=seconds)"
    printf '程序：%s\n' "${app}"
    printf 'DISPLAY=%s\n' "${DISPLAY-}"
    printf 'WAYLAND_DISPLAY=%s\n' "${WAYLAND_DISPLAY-}"
    printf 'XDG_SESSION_TYPE=%s\n' "${XDG_SESSION_TYPE-}"
    printf 'XDG_CURRENT_DESKTOP=%s\n' "${XDG_CURRENT_DESKTOP-}"
    printf 'QT_QPA_PLATFORM=%s\n' "${QT_QPA_PLATFORM-}"
    printf 'QSG_RHI_BACKEND=%s\n' "${QSG_RHI_BACKEND-}"
    collect_command uname -a
    collect_command lsb_release -a
    collect_command loginctl show-session "${XDG_SESSION_ID-}" -p Type -p Desktop -p Name -p Remote
    collect_command xrandr --current
    collect_command xdpyinfo
    collect_command glxinfo -B
    collect_command xprop -root _NET_SUPPORTING_WM_CHECK _NET_SUPPORTED
    collect_command dpkg-query -W dde-shell libdtk6declarative libdtkgui6 libqt6quick6
} >"${output_dir}/environment.log" 2>&1

run_case() {
    local name="$1"
    local render_mode="$2"
    local backend="$3"
    local case_dir="${output_dir}/${name}"
    local status
    mkdir -p "${case_dir}/screens"

    printf '[%s] 开始采集：render-mode=%s, backend=%s\n' "$(date +%H:%M:%S)" "${render_mode}" "${backend}"
    set +e
    if [[ "${backend}" == "software" ]]; then
        QT_QUICK_BACKEND=software \
        QSG_INFO=1 \
        QT_LOGGING_RULES='qt.scenegraph.general=true;qt.qpa.*=true' \
        timeout --signal=TERM 3s \
        "${app}" \
            --render-mode "${render_mode}" \
            --frame-grab on \
            --screen-grab-dir "${case_dir}/screens" \
            >"${case_dir}/run.log" 2>&1
        status=$?
    else
        QSG_RHI_BACKEND="${backend}" \
        QSG_INFO=1 \
        QT_LOGGING_RULES='qt.scenegraph.general=true;qt.qpa.*=true' \
        timeout --signal=TERM 3s \
        "${app}" \
            --render-mode "${render_mode}" \
            --frame-grab on \
            --screen-grab-dir "${case_dir}/screens" \
            >"${case_dir}/run.log" 2>&1
        status=$?
    fi
    set -e
    if [[ "${status}" -eq 124 ]]; then
        status=0
    fi

    printf '%s\n' "${status}" >"${case_dir}/exit-status.txt"
    printf '[%s] 采集完成：%s，退出码 %s，截图 %s 张\n' \
        "$(date +%H:%M:%S)" "${name}" "${status}" \
        "$(count_pngs "${case_dir}/screens")"
    return 0
}

set -e
run_case opengl-normal normal opengl
run_case opengl-node node opengl
run_case opengl-readback readback opengl
run_case software-normal normal software

tar -czf "${archive}" -C "${script_dir}" "$(basename -- "${output_dir}")"
printf '\n采集完成。请把这个文件发回来：\n%s\n' "${archive}"
