# ------------------------------------------------------------------------------
#          AR148 - Cawas Archiver
#           Static library manager for i80148 object files
#
#  Copyright (C) 2026  TheCawa <vos80584@gmail.com>
# ------------------------------------------------------------------------------
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program. If not, see <https://gnu.org>.
# ------------------------------------------------------------------------------

import sys
import struct
import os

ARCHIVE_MAGIC = b'CWA\x00'
ARCHIVE_VERSION = 1
ARCHIVE_HEADER_SIZE = 16
ARCHIVE_MEMBER_HEADER_SIZE = 40
MEMBER_NAME_LEN = 32


def _pack_fixed_string(s, length):
    b = s.encode('ascii', errors='replace')[:length - 1]
    return b + b'\x00' * (length - len(b))


def _read_fixed_string(data, offset, length):
    end = data.find(b'\x00', offset, offset + length)
    if end == -1:
        end = offset + length
    return data[offset:end].decode('ascii', errors='replace')


def create_archive(path, member_paths):
    """Create or replace an archive containing the given member files."""
    members = []
    for mp in member_paths:
        with open(mp, 'rb') as f:
            data = f.read()
        name = os.path.basename(mp)
        members.append((name, data))

    count = len(members)
    header_offset = ARCHIVE_HEADER_SIZE
    member_headers_offset = header_offset
    data_offset = member_headers_offset + count * ARCHIVE_MEMBER_HEADER_SIZE

    member_offsets = []
    current_offset = data_offset
    for _name, data in members:
        member_offsets.append(current_offset)
        current_offset += len(data)

    with open(path, 'wb') as f:
        f.write(ARCHIVE_MAGIC)
        f.write(struct.pack('>III', ARCHIVE_VERSION, count, 0))

        for (name, data), off in zip(members, member_offsets):
            f.write(_pack_fixed_string(name, MEMBER_NAME_LEN))
            f.write(struct.pack('>II', off, len(data)))

        for _name, data in members:
            f.write(data)

    print(f"Created archive '{path}' with {count} member(s)")


def list_archive(path):
    with open(path, 'rb') as f:
        data = f.read()

    if len(data) < ARCHIVE_HEADER_SIZE or data[:4] != ARCHIVE_MAGIC:
        raise ValueError(f"{path}: not a valid archive")

    version, count, _reserved = struct.unpack('>III', data[4:ARCHIVE_HEADER_SIZE])
    if version != ARCHIVE_VERSION:
        raise ValueError(f"{path}: unsupported archive version {version}")

    print(f"{path}: {count} member(s)")
    pos = ARCHIVE_HEADER_SIZE
    for i in range(count):
        name = _read_fixed_string(data, pos, MEMBER_NAME_LEN)
        offset, size = struct.unpack('>II', data[pos + MEMBER_NAME_LEN:pos + ARCHIVE_MEMBER_HEADER_SIZE])
        print(f"  {i:4d} {name:32s} offset={offset:8d} size={size:8d}")
        pos += ARCHIVE_MEMBER_HEADER_SIZE


def extract_archive(path, names=None):
    with open(path, 'rb') as f:
        data = f.read()

    if len(data) < ARCHIVE_HEADER_SIZE or data[:4] != ARCHIVE_MAGIC:
        raise ValueError(f"{path}: not a valid archive")

    version, count, _reserved = struct.unpack('>III', data[4:ARCHIVE_HEADER_SIZE])
    if version != ARCHIVE_VERSION:
        raise ValueError(f"{path}: unsupported archive version {version}")

    extracted = 0
    pos = ARCHIVE_HEADER_SIZE
    for _ in range(count):
        name = _read_fixed_string(data, pos, MEMBER_NAME_LEN)
        offset, size = struct.unpack('>II', data[pos + MEMBER_NAME_LEN:pos + ARCHIVE_MEMBER_HEADER_SIZE])
        pos += ARCHIVE_MEMBER_HEADER_SIZE

        if names is None or name in names:
            with open(name, 'wb') as out:
                out.write(data[offset:offset + size])
            print(f"Extracted: {name} ({size} bytes)")
            extracted += 1
            if names is not None:
                names.discard(name)

    if names and len(names) > 0:
        raise ValueError(f"Members not found: {', '.join(sorted(names))}")
    if extracted == 0:
        print("No members extracted")
    else:
        print(f"Extracted {extracted} member(s)")


def print_usage():
    print("Usage:")
    print("  python AR148.py rcs archive.a file1.o [file2.o ...]")
    print("  python AR148.py t archive.a")
    print("  python AR148.py x archive.a [member1 ...]")


def main():
    if len(sys.argv) < 3:
        print_usage()
        sys.exit(1)

    cmd = sys.argv[1]
    archive_path = sys.argv[2]

    try:
        if cmd == 'rcs':
            if len(sys.argv) < 4:
                print_usage()
                sys.exit(1)
            create_archive(archive_path, sys.argv[3:])
        elif cmd == 't':
            list_archive(archive_path)
        elif cmd == 'x':
            names = set(sys.argv[3:]) if len(sys.argv) > 3 else None
            extract_archive(archive_path, names)
        else:
            print(f"Unknown command: {cmd}")
            print_usage()
            sys.exit(1)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()
