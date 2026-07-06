#!/usr/bin/env bash
set -e

ROOT="${1:-$(cd "$(dirname "$0")/.." && pwd)}"
ROOT="$(cd "$ROOT" && pwd)"

EXE="$ROOT/bin/rechan"
RESDIR="$ROOT/res"
OUTDIR="$ROOT/shipping"
OUTTAR="$OUTDIR/rechan-linux64.tar.gz"
STAGEDIR="$(mktemp -d)"
PKGDIR="$STAGEDIR/rechan-linux64"
mkdir -p "$PKGDIR" "$OUTDIR"

HASH=$(sha256sum "$EXE" | awk '{print $1}')
echo "$HASH  rechan" > "$PKGDIR/SHA256SUM.txt"

cp "$EXE" "$PKGDIR/rechan"
cp -r "$RESDIR/pc" "$PKGDIR/pc"

tar -czf "$OUTTAR" -C "$PKGDIR" .

rm -rf "$STAGEDIR"
echo "Packed: $OUTTAR"
