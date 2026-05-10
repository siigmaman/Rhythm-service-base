#!/usr/bin/env bash
# Сборка и локальная установка userver (только core + зависимости по умолчанию) в .deps/userver
# для rhythm-service. Требует зависимостей из списка userver для вашей версии Ubuntu.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
USERVER_VERSION="${USERVER_VERSION:-v2.16}"
USERVER_SRC="${ROOT}/third_party/userver"
BUILD_DIR="${USERVER_SRC}/build-release"
INSTALL_PREFIX="${ROOT}/.deps/userver"

DEPS_UBUNTU_24="https://raw.githubusercontent.com/userver-framework/userver/develop/scripts/docs/en/deps/ubuntu-24.04.md"
DEPS_UBUNTU_22="https://raw.githubusercontent.com/userver-framework/userver/develop/scripts/docs/en/deps/ubuntu-22.04.md"

usage() {
  echo "Usage: $0 [--install-deps] [--fetch-only] [--build-only]"
  echo "  --install-deps  sudo apt install ... (список из userver для Ubuntu 22.04/24.04)"
  echo "  --fetch-only    только git clone userver (тег USERVER_VERSION, по умолчанию ${USERVER_VERSION})"
  echo "  --build-only    только cmake configure/build/install (каталог ${USERVER_SRC} уже есть)"
  echo ""
  echo "Переменные:"
  echo "  USERVER_VERSION           тег git, например USERVER_VERSION=v2.16"
  echo "  USERVER_BOOTSTRAP_CMAKE_ARGS  доп. флаги cmake для userver, например:"
  echo "    USERVER_BOOTSTRAP_CMAKE_ARGS='-DUSERVER_FEATURE_REDIS=ON -DUSERVER_FEATURE_POSTGRESQL=ON' $0 --build-only"
}

install_deps() {
  if ! command -v sudo >/dev/null 2>&1; then
    echo "error: sudo not found; install build dependencies manually (see README)." >&2
    exit 1
  fi
  local url
  if grep -q 'VERSION_ID="24.04"' /etc/os-release 2>/dev/null; then
    url="$DEPS_UBUNTU_24"
  elif grep -q 'VERSION_ID="22.04"' /etc/os-release 2>/dev/null; then
    url="$DEPS_UBUNTU_22"
  else
    echo "error: unsupported Ubuntu (need 22.04 or 24.04 in /etc/os-release)." >&2
    exit 1
  fi
  echo "Fetching package list from: $url"
  local pkgs
  pkgs="$(wget -q -O - "$url" | sed '/^#/d;/^$/d' | tr '\n' ' ')"
  sudo apt-get update -qq
  sudo apt-get install -y --allow-downgrades ${pkgs}
}

fetch_userver() {
  mkdir -p "${ROOT}/third_party"
  if [[ -d "${USERVER_SRC}/.git" ]]; then
    echo "userver already present: ${USERVER_SRC}"
    return 0
  fi
  git clone --branch "${USERVER_VERSION}" --depth 1 \
    https://github.com/userver-framework/userver.git "${USERVER_SRC}"
}

build_userver() {
  if [[ ! -f "${USERVER_SRC}/CMakeLists.txt" ]]; then
    echo "error: userver sources missing at ${USERVER_SRC}; run without --build-only first." >&2
    exit 1
  fi
  # Первый прогон без apt-зависимостей оставляет в кэше BOOST_CPM=ON: тогда userver больше не
  # ищет системный Boost и тянет его через CPM. При USERVER_INSTALL=ON это ломает generate:
  # install(EXPORT "userver-targets" ...) / boost_* "not in any export set".
  if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]] && grep -q '^BOOST_CPM:BOOL=ON' "${BUILD_DIR}/CMakeCache.txt"; then
    echo "note: removing ${BUILD_DIR} (stale BOOST_CPM=ON; need system Boost for USERVER_INSTALL)."
    rm -rf "${BUILD_DIR}"
  fi
  # shellcheck disable=SC2086
  cmake -S "${USERVER_SRC}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DUSERVER_INSTALL=ON \
    -DUSERVER_BUILD_TESTS=OFF \
    -DUSERVER_BUILD_SAMPLES=OFF \
    -DUSERVER_FEATURE_TESTSUITE=OFF \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    ${USERVER_BOOTSTRAP_CMAKE_ARGS:-}
  cmake --build "${BUILD_DIR}" -j"$(nproc)"
  cmake --install "${BUILD_DIR}"
}

DO_INSTALL_DEPS=0
DO_FETCH_ONLY=0
DO_BUILD_ONLY=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --install-deps) DO_INSTALL_DEPS=1 ;;
    --fetch-only) DO_FETCH_ONLY=1 ;;
    --build-only) DO_BUILD_ONLY=1 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "unknown option: $1" >&2; usage; exit 1 ;;
  esac
  shift
done

if (( DO_INSTALL_DEPS )); then
  install_deps
fi

if (( DO_BUILD_ONLY )); then
  build_userver
else
  fetch_userver
  if ! (( DO_FETCH_ONLY )); then
    build_userver
  fi
fi

echo ""
echo "userver -> ${INSTALL_PREFIX}"
echo "Дальше: cmake -S \"${ROOT}\" -B \"${ROOT}/build\" -DCMAKE_BUILD_TYPE=Release -Duserver_DIR=\"${INSTALL_PREFIX}/lib/cmake/userver\""
echo "         cmake --build \"${ROOT}/build\" -j\"\$(nproc)\""
echo "Postgres+Redis: задайте USERVER_BOOTSTRAP_CMAKE_ARGS с USERVER_FEATURE_POSTGRESQL/REDIS, пересоберите; см. README."
