#! /bin/bash

set -euo pipefail

BASE_PATH="$(dirname "$0")/../../.."
cd "$BASE_PATH"

INPUT="$1"
shift 1
OUTPUT_NAME="$(basename "$INPUT")"
OUTPUT_PATH="out/${OUTPUT_NAME%.*}.midi"

mkdir -p out
cat "$INPUT" | ".build/Flex-Bison-Compiler" "$@"
mv output.mid "$OUTPUT_PATH"
