set -e

MODE="${1:-release}"
case "$MODE" in
    release)  DEBUG=0 ;;
    shipping) DEBUG=0 ;;
    debug)    DEBUG=1 ;;
    *)
        echo "Usage: $0 [release|debug|shipping]"
        exit 1
        ;;
esac

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MAKE="$REPO_ROOT/vendor/devkitpro/make/make.exe"

if [ ! -x "$MAKE" ]; then
    echo "devkitA64 toolchain not found at $MAKE"
    echo "Run scripts/fetch-devkitpro.sh first."
    exit 1
fi

echo "=== Building libp3d ($MODE) ==="
(cd "$REPO_ROOT/switch/libp3d" && "$MAKE" DEBUG=$DEBUG)

echo "=== Building rechan ($MODE) ==="
if [ "$DEBUG" = "1" ]; then
    OUT_ELF="$REPO_ROOT/switch/rechan/rechan-debug.elf"
    OUT_NRO="$REPO_ROOT/switch/rechan/rechan-debug.nro"
else
    OUT_ELF="$REPO_ROOT/switch/rechan/rechan.elf"
    OUT_NRO="$REPO_ROOT/switch/rechan/rechan.nro"
fi
# Force a relink: make doesn't always notice libp3d.a changed underneath it.
rm -f "$OUT_ELF" "$OUT_NRO"
(cd "$REPO_ROOT/switch/rechan" && "$MAKE" DEBUG=$DEBUG)

echo
echo "Built: $OUT_NRO"

if [ "$MODE" = "shipping" ]; then
    echo
    echo "=== Packing shipping zip ==="
    powershell.exe -ExecutionPolicy Bypass -File "$REPO_ROOT/scripts/pack-switch.ps1" -Root "$REPO_ROOT"
fi
