#!/usr/bin/env bash
# Build and install HPX locally (not available via Homebrew).
# Default prefix: ~/cpp-deps/hpx
set -euo pipefail

HPX_ROOT="${HPX_ROOT:-$HOME/cpp-deps/hpx}"
SRC="${HPX_SRC:-$HOME/cpp-deps/hpx-src}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)}"
REPO="${HPX_REPO:-https://github.com/STEllAR-GROUP/hpx.git}"
# master has modern Asio fixes; v1.11.0 breaks with Homebrew Asio 1.36
REF="${HPX_REF:-master}"

echo "HPX_ROOT=$HPX_ROOT"
echo "SRC=$SRC"
echo "REF=$REF"

mkdir -p "$(dirname "$SRC")"
if [[ ! -d "$SRC/.git" ]]; then
  git clone --depth 1 --branch "$REF" "$REPO" "$SRC" \
    || git clone --depth 1 "$REPO" "$SRC"
else
  git -C "$SRC" fetch --depth 1 origin "$REF" || true
  git -C "$SRC" checkout "$REF" || git -C "$SRC" checkout FETCH_HEAD || true
fi

# Ensure hwloc is present (Homebrew)
if ! brew list hwloc >/dev/null 2>&1; then
  brew install hwloc
fi

rm -rf "$SRC/build"
cmake -S "$SRC" -B "$SRC/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$HPX_ROOT" \
  -DHwloc_ROOT="$(brew --prefix hwloc)" \
  -DHPX_WITH_EXAMPLES=OFF \
  -DHPX_WITH_TESTS=OFF \
  -DHPX_WITH_BENCHMARKS=OFF \
  -DHPX_WITH_DOCUMENTATION=OFF \
  -DHPX_WITH_TOOLS=OFF \
  -DHPX_WITH_NETWORKING=OFF \
  -DHPX_WITH_DISTRIBUTED_RUNTIME=OFF \
  -DHPX_WITH_FETCH_ASIO=ON \
  -DHPX_WITH_MALLOC=system

# Prefer HPX-fetched Asio over Homebrew Asio headers
ASIO_HPP=$(find "$SRC/build/_deps" -name asio.hpp 2>/dev/null | head -1 || true)
if [[ -n "$ASIO_HPP" ]]; then
  ASIO_INC=$(dirname "$ASIO_HPP")
  echo "Using fetched Asio include: $ASIO_INC"
  cmake -S "$SRC" -B "$SRC/build" -DCMAKE_CXX_FLAGS="-isystem ${ASIO_INC}"
fi

cmake --build "$SRC/build" -j"$JOBS"
cmake --install "$SRC/build"

if [[ -f "$HPX_ROOT/lib/cmake/HPX/HPXConfig.cmake" ]]; then
  echo "HPX_INSTALL_OK -> $HPX_ROOT"
else
  echo "ERROR: HPXConfig.cmake not found after install" >&2
  exit 1
fi
