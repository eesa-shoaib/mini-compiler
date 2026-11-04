#!/usr/bin/env bash
#
# Build and run the C-- front-end.
#
# Usage:
#   ./run.sh                      # parse examples/hello.cmm
#   ./run.sh path/to/file.cmm     # parse a given file
#   ./run.sh file.cmm --tokens    # dump the token stream instead

set -euo pipefail
cd "$(dirname "$0")"

file="examples/hello.cmm"
tokens=0

for arg in "$@"; do
    case "$arg" in
        --tokens) tokens=1 ;;
        -h|--help)
            sed -n '3,9p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        -*)
            echo "Unknown option: $arg" >&2
            exit 2
            ;;
        *)
            file="$arg"
            ;;
    esac
done

if [[ ! -f "$file" ]]; then
    echo "No such file: $file" >&2
    exit 1
fi

make --no-print-directory

if (( tokens )); then
    g++ -Isrc -std=c++17 -Wall -Wextra -O2 src/lexer.cpp -o build/cmm-lex
    exec ./build/cmm-lex "$file"
else
    exec ./build/cmmc "$file"
fi
