#!/usr/bin/env bash
set -euo pipefail

in="$1"
out="$2"

cp "$in" "$out"

sed -n 's/.*\(_mangle_([^"]*"[^"]*")\).*/\1/p' "$in" |
sort -u |
while read -r macro; do
    ascii=$(sed 's/.*"\([^"]*\)".*/\1/' <<< "$macro")
    mangled=$(./mangler "$ascii")

    sed -i "s|$macro|$mangled|g" "$out"
done
