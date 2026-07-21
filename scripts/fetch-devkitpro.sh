#!/bin/sh

set -e

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$REPO_ROOT/vendor/devkitpro"
CACHE="$DEST/_pkgcache"
UA="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36"

BASE="https://pkg.devkitpro.org/packages"
PACKAGES="
windows/x86_64/devkita64-gcc-16.1.0-1-x86_64.pkg.tar.zst
windows/x86_64/devkita64-binutils-2.46.0-1-x86_64.pkg.tar.zst
devkita64-newlib-4.6.0.20260123-4-any.pkg.tar.zst
devkita64-rules-1.1.1-1-any.pkg.tar.zst
libnx-4.12.0-1-any.pkg.tar.zst
windows/x86_64/switch-tools-1.13.1-1-x86_64.pkg.tar.zst
switch-mesa-20.1.0-5-any.pkg.tar.zst
switch-libdrm_nouveau-1.0.1-2-any.pkg.tar.xz
"

mkdir -p "$CACHE"
cd "$CACHE"

extract_pkg() {
    pkg_file="$1"
    case "$pkg_file" in
        *.zst) tar_file="${pkg_file%.zst}" ;;
        *.xz)  tar_file="${pkg_file%.xz}" ;;
        *) echo "error: unrecognized package compression for $pkg_file" >&2; exit 1 ;;
    esac

    if tar --zstd -tf "$pkg_file" >/dev/null 2>&1 || tar -tf "$pkg_file" >/dev/null 2>&1; then
        # GNU tar with built-in zstd/xz (liblzma) support -- handles both.
        tar -xf "$pkg_file" -C "$DEST" --strip-components=2 --wildcards 'opt/devkitpro/*'
    elif command -v zstd >/dev/null 2>&1 && [ "${pkg_file%.zst}" != "$pkg_file" ]; then
        zstd -d -f "$pkg_file" -o "$tar_file"
        tar -xf "$tar_file" -C "$DEST" --strip-components=2 --wildcards 'opt/devkitpro/*'
        rm -f "$tar_file"
    elif command -v xz >/dev/null 2>&1 && [ "${pkg_file%.xz}" != "$pkg_file" ]; then
        xz -d -f -k "$pkg_file"
        tar -xf "$tar_file" -C "$DEST" --strip-components=2 --wildcards 'opt/devkitpro/*'
        rm -f "$tar_file"
    elif command -v 7z >/dev/null 2>&1 || command -v 7za >/dev/null 2>&1; then
        SEVENZ="$(command -v 7z || command -v 7za)"
        "$SEVENZ" x "$pkg_file" -y >/dev/null
        tar -xf "$tar_file" -C "$DEST" --strip-components=2 --wildcards 'opt/devkitpro/*'
        rm -f "$tar_file"
    else
        echo "error: need a zstd/xz-capable tar, standalone zstd/xz binaries, or 7z/7za on PATH" >&2
        exit 1
    fi
}

for pkg_path in $PACKAGES; do
    pkg_file="$(basename "$pkg_path")"
    echo "=== fetching $pkg_file ==="
    curl -fL -A "$UA" -o "$pkg_file" "$BASE/$pkg_path"
    echo "=== extracting $pkg_file ==="
    extract_pkg "$pkg_file"
    rm -f "$pkg_file"
done

if [ ! -f "$DEST/make/make.exe" ]; then
    echo "=== fetching make.exe (via chocolatey's make nupkg) ==="
    curl -fsSL -A "$UA" -o make.nupkg "https://community.chocolatey.org/api/v2/package/make/"
    mkdir -p "$DEST/make" make_nupkg_extract
    if command -v 7z >/dev/null 2>&1 || command -v 7za >/dev/null 2>&1; then
        SEVENZ="$(command -v 7z || command -v 7za)"
        "$SEVENZ" x make.nupkg -y -o"make_nupkg_extract" >/dev/null
    else
        unzip -q make.nupkg -d make_nupkg_extract
    fi
    find make_nupkg_extract -iname make.exe -exec cp {} "$DEST/make/make.exe" \;
    rm -rf make.nupkg make_nupkg_extract
fi

cd "$REPO_ROOT"
rm -rf "$CACHE"

echo
echo "devkitA64/libnx/switch-tools/switch-mesa/make extracted into $DEST"
"$DEST/devkitA64/bin/aarch64-none-elf-g++.exe" --version 2>/dev/null \
    || "$DEST/devkitA64/bin/aarch64-none-elf-g++" --version
"$DEST/make/make.exe" --version 2>/dev/null | head -1
ls "$DEST/portlibs/switch/lib/libEGL.a" >/dev/null 2>&1 && echo "switch-mesa EGL/GLES libs present"
