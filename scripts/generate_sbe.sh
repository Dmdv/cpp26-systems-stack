#!/usr/bin/env bash
# Regenerate Real Logic SBE C++ codecs from schemas/sbe-market-data-schema.xml
#
# Requires: Java 11+, curl
# Usage:   ./scripts/generate_sbe.sh
# Output:  generated/sbe/market_sbe/{Tick,MessageHeader}.h
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SCHEMA="${ROOT}/schemas/sbe-market-data-schema.xml"
OUT_DIR="${ROOT}/generated/sbe"
SBE_VERSION="${SBE_VERSION:-1.34.1}"
JAR_DIR="${ROOT}/.tools"
JAR="${JAR_DIR}/sbe-all-${SBE_VERSION}.jar"
JAR_URL="https://repo1.maven.org/maven2/uk/co/real-logic/sbe-all/${SBE_VERSION}/sbe-all-${SBE_VERSION}.jar"

if [[ ! -f "${SCHEMA}" ]]; then
  echo "error: schema not found: ${SCHEMA}" >&2
  exit 1
fi

if ! command -v java >/dev/null 2>&1; then
  echo "error: java not found (install JDK 11+)" >&2
  exit 1
fi

mkdir -p "${JAR_DIR}" "${OUT_DIR}"
if [[ ! -f "${JAR}" ]]; then
  echo "downloading ${JAR_URL}"
  curl -fsSL -o "${JAR}" "${JAR_URL}"
fi

echo "generating SBE C++ from ${SCHEMA}"
rm -rf "${OUT_DIR}/market_sbe"
java -Dsbe.output.dir="${OUT_DIR}" \
     -Dsbe.target.language=Cpp \
     -jar "${JAR}" \
     "${SCHEMA}"

echo "wrote:"
find "${OUT_DIR}" -type f -name '*.h' | sort
echo "done. Commit generated headers if the schema changed."
