#!/usr/bin/env python3
from __future__ import annotations

import argparse
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


WHITELIST_DIRS = {
    "fe",
    "rchars",
    "rtarget",
    "scr",
    "sound",
    "tim",
    "xc",
}

WHITELIST_ROOT_FILES = {
    "license.tim",
    "license_data.dat",
    "loadanim.con",
    "postdemo.tim",
    "predemo.tim",
    "runfirst.tim",
}

RAW_XA_EXTENSIONS = {
    ".str",
}


@dataclass
class DirEntry:
    path_parts: list[str]
    is_dir: bool
    extent_lba: int
    size: int


@dataclass
class SectorLayout:
    name: str
    sector_size: int
    data_offset: int
    data_size: int
    raw_kind: str


def read_u32_le(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def safe_lower_name(name: str) -> str:
    if ";" in name:
        name = name.split(";", 1)[0]
    return name.strip().lower()


def is_dot_entry(name_bytes: bytes) -> bool:
    return name_bytes in (b"\x00", b"\x01")


def ensure_parent_dir(path: Path) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)


def should_extract(path_parts: list[str]) -> bool:
    if not path_parts:
        return False

    if len(path_parts) == 1:
        return path_parts[0] in WHITELIST_ROOT_FILES

    return path_parts[0] in WHITELIST_DIRS


def should_extract_as_raw_xa(path_parts: list[str]) -> bool:
    if not path_parts:
        return False
    return Path(path_parts[-1]).suffix.lower() in RAW_XA_EXTENSIONS


class IsoBinReader:
    def __init__(self, bin_path: Path):
        self.bin_path = bin_path
        self.fp = open(bin_path, "rb")
        self.file_size = bin_path.stat().st_size
        self.layout = self._detect_layout()

    def close(self) -> None:
        self.fp.close()

    def _detect_layout(self) -> SectorLayout:
        candidates = [
            SectorLayout("2048", 2048, 0, 2048, "2048"),
            SectorLayout("2352_mode1", 2352, 16, 2048, "2352"),
            SectorLayout("2352_xa", 2352, 24, 2048, "2352"),
            SectorLayout("2336_xa", 2336, 8, 2048, "2336"),
        ]

        for layout in candidates:
            try:
                pvd = self.read_logical_block(16, layout)
            except Exception:
                continue

            if len(pvd) >= 7 and pvd[0] == 0x01 and pvd[1:6] == b"CD001" and pvd[6] == 0x01:
                return layout

        raise RuntimeError(
            "Could not detect a supported PSX BIN layout. "
            "Supported layouts: 2048, 2352 Mode1, 2352 XA, 2336 XA."
        )

    def read_logical_block(self, lba: int, layout: SectorLayout | None = None) -> bytes:
        if layout is None:
            layout = self.layout

        offset = lba * layout.sector_size + layout.data_offset
        self.fp.seek(offset)
        data = self.fp.read(layout.data_size)
        if len(data) != layout.data_size:
            raise RuntimeError(f"Failed reading logical block {lba}")
        return data

    def read_extent_logical(self, extent_lba: int, size: int) -> bytes:
        blocks = (size + 2047) // 2048
        out = bytearray()

        for i in range(blocks):
            out.extend(self.read_logical_block(extent_lba + i))

        return bytes(out[:size])

    def read_raw_sector(self, lba: int) -> bytes:
        offset = lba * self.layout.sector_size
        self.fp.seek(offset)
        data = self.fp.read(self.layout.sector_size)
        if len(data) != self.layout.sector_size:
            raise RuntimeError(f"Failed reading raw sector {lba}")
        return data

    def raw_sector_to_xa2336(self, raw_sector: bytes) -> bytes:
        if self.layout.raw_kind == "2352":
            return raw_sector[16:16 + 2336]
        if self.layout.raw_kind == "2336":
            return raw_sector
        raise RuntimeError("Cannot preserve raw XA sectors from a 2048-only image")

    def read_extent_raw_xa2336_by_sector_count(self, extent_lba: int, sector_count: int) -> bytes:
        if self.layout.raw_kind == "2048":
            raise RuntimeError("This BIN layout has no raw XA sectors to preserve")

        out = bytearray()

        for i in range(sector_count):
            raw_sector = self.read_raw_sector(extent_lba + i)
            out.extend(self.raw_sector_to_xa2336(raw_sector))

        return bytes(out)


def parse_dir_records(data: bytes, parent_parts: list[str]) -> list[DirEntry]:
    entries: list[DirEntry] = []
    i = 0
    total = len(data)

    while i < total:
        record_len = data[i]

        if record_len == 0:
            next_block = ((i // 2048) + 1) * 2048
            if next_block <= i:
                break
            i = next_block
            continue

        if i + record_len > total:
            break

        rec = data[i:i + record_len]
        if len(rec) < 34:
            break

        extent_lba = read_u32_le(rec, 2)
        file_size = read_u32_le(rec, 10)
        file_flags = rec[25]
        file_id_len = rec[32]

        if 33 + file_id_len > len(rec):
            i += record_len
            continue

        file_id = rec[33:33 + file_id_len]

        if is_dot_entry(file_id):
            i += record_len
            continue

        try:
            name = file_id.decode("ascii", errors="replace")
        except Exception:
            name = file_id.decode("latin-1", errors="replace")

        name = safe_lower_name(name)
        is_dir = (file_flags & 0x02) != 0

        entries.append(
            DirEntry(
                path_parts=parent_parts + [name],
                is_dir=is_dir,
                extent_lba=extent_lba,
                size=file_size,
            )
        )

        i += record_len

    return entries


def read_root(reader: IsoBinReader) -> DirEntry:
    pvd = reader.read_logical_block(16)

    if not (len(pvd) >= 190 and pvd[0] == 1 and pvd[1:6] == b"CD001"):
        raise RuntimeError("Primary Volume Descriptor not found")

    root_record_len = pvd[156]
    if root_record_len < 34:
        raise RuntimeError("Invalid root directory record")

    root_rec = pvd[156:156 + root_record_len]

    return DirEntry(
        path_parts=[],
        is_dir=True,
        extent_lba=read_u32_le(root_rec, 2),
        size=read_u32_le(root_rec, 10),
    )


def walk_tree(reader: IsoBinReader, dir_entry: DirEntry) -> Iterable[DirEntry]:
    dir_data = reader.read_extent_logical(dir_entry.extent_lba, dir_entry.size)
    children = parse_dir_records(dir_data, dir_entry.path_parts)

    for child in children:
        yield child
        if child.is_dir:
            yield from walk_tree(reader, child)


def extract_logical(reader: IsoBinReader, entry: DirEntry, out_path: Path) -> None:
    data = reader.read_extent_logical(entry.extent_lba, entry.size)
    ensure_parent_dir(out_path)
    with open(out_path, "wb") as f:
        f.write(data)


def extract_raw_xa(reader: IsoBinReader, entry: DirEntry, next_extent_lba: int, out_path: Path) -> None:
    sector_count = next_extent_lba - entry.extent_lba
    if sector_count <= 0:
        raise RuntimeError("Invalid STR sector span")

    data = reader.read_extent_raw_xa2336_by_sector_count(entry.extent_lba, sector_count)
    ensure_parent_dir(out_path)
    with open(out_path, "wb") as f:
        f.write(data)


def run_extraction(bin_path: Path, out_dir: Path, quiet: bool = False) -> int:
    reader = IsoBinReader(bin_path)

    try:
        root = read_root(reader)
        all_entries = list(walk_tree(reader, root))
        file_entries = [e for e in all_entries if not e.is_dir]
        file_entries.sort(key=lambda e: e.extent_lba)

        next_extent_map: dict[int, int] = {}
        for i, entry in enumerate(file_entries):
            if i + 1 < len(file_entries):
                next_extent_map[id(entry)] = file_entries[i + 1].extent_lba
            else:
                blocks = (entry.size + 2047) // 2048
                next_extent_map[id(entry)] = entry.extent_lba + max(blocks, 1)

        if not quiet:
            print(f"Detected layout: {reader.layout.name}")
            print(f"Input:  {bin_path}")
            print(f"Output: {out_dir}")
            print()

        extracted = 0

        for entry in file_entries:
            if not should_extract(entry.path_parts):
                continue

            out_path = out_dir.joinpath(*entry.path_parts)

            if should_extract_as_raw_xa(entry.path_parts):
                try:
                    extract_raw_xa(reader, entry, next_extent_map[id(entry)], out_path)
                    mode = "raw-xa"
                except Exception as e:
                    extract_logical(reader, entry, out_path)
                    mode = f"logical-fallback ({e})"
            else:
                extract_logical(reader, entry, out_path)
                mode = "logical"

            extracted += 1

            if not quiet:
                print(f"[extract:{mode}] {'/'.join(entry.path_parts)} -> {out_path.relative_to(out_dir).as_posix()}")

        return extracted

    finally:
        reader.close()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Extract selected files from a PSX .bin image.")
    parser.add_argument("bin", type=Path, help="Input PSX .bin file")
    parser.add_argument("output", type=Path, help="Output directory")
    parser.add_argument("--quiet", action="store_true", help="Less console output")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    bin_path = args.bin.resolve()
    out_dir = args.output.resolve()

    if not bin_path.exists():
        print(f"Error: input file does not exist: {bin_path}", file=sys.stderr)
        return 1

    if bin_path.suffix.lower() != ".bin":
        print("Error: input must be a .bin file", file=sys.stderr)
        return 1

    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        count = run_extraction(bin_path, out_dir, quiet=args.quiet)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1

    print()
    print(f"Done. Extracted {count} file(s) to:")
    print(out_dir)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())