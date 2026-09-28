"""Create the version-locked NPDM overlay needed by the phase-4 hook.

The input is never modified.  This preserves Stardew Valley's original NPDM
and adds only svcMapProcessMemory (0x74) and svcUnmapProcessMemory (0x75) to
both the ACID restriction set and the ACI0 process capability set.
"""

from __future__ import annotations

import argparse
import hashlib
import struct
from pathlib import Path


EXPECTED_SHA256 = "38DB701EC4BA0A688A96F413238FCCBB751D0CEACC8D12D0AB2F077E07562CCE"
REQUIRED_SYSCALLS = (0x74, 0x75)


def u32(data: bytes | bytearray, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def p32(data: bytearray, offset: int, value: int) -> None:
    struct.pack_into("<I", data, offset, value)


def syscall_descriptor(syscalls: tuple[int, ...]) -> int:
    groups = {svc // 24 for svc in syscalls}
    if len(groups) != 1:
        raise ValueError("all syscalls in one descriptor must share a 24-SVC group")
    group = groups.pop()
    mask = 0
    for svc in syscalls:
        mask |= 1 << (svc % 24)
    return (group << 29) | (mask << 5) | 0xF


def decode_syscalls(words: list[int]) -> set[int]:
    result: set[int] = set()
    for word in words:
        if word & 0x1F != 0x0F:
            continue
        group = word >> 29
        mask = (word >> 5) & 0xFFFFFF
        for bit in range(24):
            if mask & (1 << bit):
                result.add(group * 24 + bit)
    return result


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()

    original = args.input.read_bytes()
    digest = hashlib.sha256(original).hexdigest().upper()
    if digest != EXPECTED_SHA256:
        raise ValueError(
            f"refusing to patch unexpected NPDM: SHA-256 {digest}, "
            f"expected {EXPECTED_SHA256}"
        )
    if original[:4] != b"META":
        raise ValueError("input is not an NPDM META image")

    data = bytearray(original)
    aci0_offset, aci0_size, acid_offset, acid_size = struct.unpack_from(
        "<IIII", data, 0x70
    )
    acid_header = acid_offset + 0x200
    if data[aci0_offset:aci0_offset + 4] != b"ACI0":
        raise ValueError("invalid ACI0 offset")
    if data[acid_header:acid_header + 4] != b"ACID":
        raise ValueError("invalid ACID offset")

    descriptor = syscall_descriptor(REQUIRED_SYSCALLS)
    sections = (
        ("ACID", acid_offset, acid_header, 0x7C),
        ("ACI0", aci0_offset, aci0_offset, 0x74),
    )

    for name, root, header, meta_size_offset in sections:
        cap_offset = u32(data, header + 0x30)
        cap_size = u32(data, header + 0x34)
        cap_start = root + cap_offset
        cap_end = cap_start + cap_size
        words = list(struct.unpack_from(f"<{cap_size // 4}I", data, cap_start))
        existing = decode_syscalls(words)
        if any(svc in existing for svc in REQUIRED_SYSCALLS):
            raise ValueError(f"{name} already contains one of the requested SVCs")

        if name == "ACID":
            if cap_end != acid_offset + acid_size:
                raise ValueError("unexpected ACID capability layout")
            if aci0_offset - cap_end < 4:
                raise ValueError("no padding available after ACID capabilities")
            p32(data, cap_end, descriptor)
            p32(data, header + 0x04, u32(data, header + 0x04) + 4)
            acid_size += 4
            p32(data, meta_size_offset, acid_size)
        else:
            if cap_end != len(data):
                raise ValueError("unexpected ACI0 capability layout")
            data.extend(struct.pack("<I", descriptor))
            aci0_size += 4
            p32(data, meta_size_offset, aci0_size)

        p32(data, header + 0x34, cap_size + 4)

    # The original signature no longer authenticates the modified ACID. Loose
    # NPDM overlays produced by the exlaunch toolchain are unsigned as well.
    data[acid_offset:acid_offset + 0x200] = bytes(0x200)

    # Re-parse both capability arrays and refuse to emit a partial patch.
    for name, root, header, _meta_size_offset in sections:
        cap_offset = u32(data, header + 0x30)
        cap_size = u32(data, header + 0x34)
        words = list(struct.unpack_from(f"<{cap_size // 4}I", data, root + cap_offset))
        missing = set(REQUIRED_SYSCALLS) - decode_syscalls(words)
        if missing:
            raise ValueError(f"{name} is missing patched SVCs: {sorted(missing)}")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"Patched NPDM: {args.output}")
    print(f"SHA-256: {hashlib.sha256(data).hexdigest().upper()}")
    print("Added SVCs to ACID and ACI0: 0x74, 0x75")


if __name__ == "__main__":
    main()
