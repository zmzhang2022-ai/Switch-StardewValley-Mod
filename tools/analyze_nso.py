#!/usr/bin/env python3
"""Read-only NSO0 structure, string, and basic ARM64 XREF analyzer.

This script intentionally has no third-party dependencies.  It supports the
raw LZ4 blocks used by compressed NSO segments and reports module-relative
virtual addresses (offsets from the NSO load base), never guessed runtime
addresses.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Sequence, Tuple


NSO_HEADER_SIZE = 0x100


def u32(data: bytes, offset: int) -> int:
    return struct.unpack_from("<I", data, offset)[0]


def sign_extend(value: int, bits: int) -> int:
    sign = 1 << (bits - 1)
    return (value ^ sign) - sign


def lz4_block_decompress(source: bytes, expected_size: int) -> bytes:
    """Decode one raw LZ4 block (the representation used in NSO files)."""
    src = memoryview(source)
    out = bytearray()
    pos = 0

    while pos < len(src):
        token = src[pos]
        pos += 1

        literal_len = token >> 4
        if literal_len == 15:
            while True:
                if pos >= len(src):
                    raise ValueError("truncated LZ4 literal length")
                extra = src[pos]
                pos += 1
                literal_len += extra
                if extra != 255:
                    break

        literal_end = pos + literal_len
        if literal_end > len(src):
            raise ValueError("truncated LZ4 literal bytes")
        out.extend(src[pos:literal_end])
        pos = literal_end

        # A final literal-only sequence has no match offset.
        if pos == len(src):
            break
        if pos + 2 > len(src):
            raise ValueError("truncated LZ4 match offset")

        match_offset = src[pos] | (src[pos + 1] << 8)
        pos += 2
        if match_offset == 0 or match_offset > len(out):
            raise ValueError(f"invalid LZ4 match offset {match_offset:#x}")

        match_len = token & 0x0F
        if match_len == 15:
            while True:
                if pos >= len(src):
                    raise ValueError("truncated LZ4 match length")
                extra = src[pos]
                pos += 1
                match_len += extra
                if extra != 255:
                    break
        match_len += 4

        # Repeating the already-decoded match period handles overlapping LZ4
        # copies while remaining much faster than appending one byte at a time.
        period = bytes(out[-match_offset:])
        repeats = (match_len + match_offset - 1) // match_offset
        out.extend((period * repeats)[:match_len])

    if len(out) != expected_size:
        raise ValueError(
            f"LZ4 size mismatch: got {len(out):#x}, expected {expected_size:#x}"
        )
    return bytes(out)


@dataclass(frozen=True)
class Segment:
    name: str
    file_offset: int
    memory_offset: int
    decompressed_size: int
    compressed_size: int
    expected_hash: bytes
    compressed: bool
    hash_enabled: bool

    @property
    def memory_end(self) -> int:
        return self.memory_offset + self.decompressed_size


@dataclass(frozen=True)
class CandidateString:
    segment: str
    address: int
    text: str
    matched_needles: Tuple[str, ...]


def parse_header(header: bytes) -> Tuple[int, bytes, str, List[Segment], Dict[str, Tuple[int, int]]]:
    if len(header) != NSO_HEADER_SIZE or header[:4] != b"NSO0":
        raise ValueError("not an NSO0 file")

    flags = u32(header, 0x0C)
    file_offsets = (u32(header, 0x10), u32(header, 0x20), u32(header, 0x30))
    memory_offsets = (u32(header, 0x14), u32(header, 0x24), u32(header, 0x34))
    decompressed_sizes = (u32(header, 0x18), u32(header, 0x28), u32(header, 0x38))
    compressed_sizes = (u32(header, 0x60), u32(header, 0x64), u32(header, 0x68))
    names = ("text", "rodata", "data")
    hashes = (header[0xA0:0xC0], header[0xC0:0xE0], header[0xE0:0x100])

    segments = []
    for index, name in enumerate(names):
        segments.append(
            Segment(
                name=name,
                file_offset=file_offsets[index],
                memory_offset=memory_offsets[index],
                decompressed_size=decompressed_sizes[index],
                compressed_size=compressed_sizes[index],
                expected_hash=hashes[index],
                compressed=bool(flags & (1 << index)),
                hash_enabled=bool(flags & (1 << (index + 3))),
            )
        )

    module_id = header[0x40:0x60]
    module_name_offset = u32(header, 0x1C)
    module_name_size = u32(header, 0x2C)
    extents = {
        "api_info": (u32(header, 0x88), u32(header, 0x8C)),
        "dynstr": (u32(header, 0x90), u32(header, 0x94)),
        "dynsym": (u32(header, 0x98), u32(header, 0x9C)),
    }
    return flags, module_id, f"{module_name_offset:#x}:{module_name_size:#x}", segments, extents


def load_segment(file_data: bytes, segment: Segment) -> bytes:
    stored_size = segment.compressed_size if segment.compressed else segment.decompressed_size
    end = segment.file_offset + stored_size
    if end > len(file_data):
        raise ValueError(f"{segment.name} extends beyond EOF")
    stored = file_data[segment.file_offset:end]
    if segment.compressed:
        return lz4_block_decompress(stored, segment.decompressed_size)
    return stored


def iter_ascii_strings(data: bytes, minimum: int = 4) -> Iterable[Tuple[int, str]]:
    pattern = re.compile(rb"[\x09\x20-\x7e]{%d,}" % minimum)
    for match in pattern.finditer(data):
        yield match.start(), match.group().decode("ascii", errors="replace")


def parse_dynsym(rodata: bytes, extents: Dict[str, Tuple[int, int]]) -> List[Tuple[str, int, int, int]]:
    str_offset, str_size = extents["dynstr"]
    sym_offset, sym_size = extents["dynsym"]
    strings = rodata[str_offset:str_offset + str_size]
    symbols = rodata[sym_offset:sym_offset + sym_size]
    if len(symbols) % 24 != 0:
        raise ValueError(f"dynsym size {len(symbols):#x} is not a multiple of 24")
    result = []
    for offset in range(0, len(symbols), 24):
        name_offset, info, _other, section, value, size = struct.unpack_from(
            "<IBBHQQ", symbols, offset
        )
        if name_offset >= len(strings):
            name = f"<bad-str-offset:{name_offset:#x}>"
        else:
            end = strings.find(b"\0", name_offset)
            if end < 0:
                end = len(strings)
            name = strings[name_offset:end].decode("utf-8", errors="replace")
        result.append((name, section, value, size))
    return result


def find_candidate_strings(
    loaded: Dict[str, bytes], segments: Sequence[Segment], needles: Sequence[str]
) -> List[CandidateString]:
    lower_needles = tuple(n.lower() for n in needles)
    by_name = {segment.name: segment for segment in segments}
    found: List[CandidateString] = []
    for segment_name in ("rodata", "data", "text"):
        blob = loaded[segment_name]
        base = by_name[segment_name].memory_offset
        for offset, value in iter_ascii_strings(blob):
            lower_value = value.lower()
            matches = tuple(
                needles[index]
                for index, needle in enumerate(lower_needles)
                if needle in lower_value
            )
            if matches:
                found.append(CandidateString(segment_name, base + offset, value, matches))
    return found


def utf16le_ascii_context(blob: bytes, offset: int, encoded_size: int) -> Tuple[int, str]:
    start = offset
    while start >= 2:
        codepoint = blob[start - 2] | (blob[start - 1] << 8)
        if not (0x20 <= codepoint <= 0x7E):
            break
        start -= 2
    end = offset + encoded_size
    while end + 1 < len(blob):
        codepoint = blob[end] | (blob[end + 1] << 8)
        if not (0x20 <= codepoint <= 0x7E):
            break
        end += 2
    return start, blob[start:end].decode("utf-16le", errors="replace")


def summarize_utf16le_needles(
    loaded: Dict[str, bytes], segments: Sequence[Segment], needles: Sequence[str]
) -> Dict[str, Tuple[int, List[Tuple[str, int, str]]]]:
    by_name = {segment.name: segment for segment in segments}
    result: Dict[str, Tuple[int, List[Tuple[str, int, str]]]] = {}
    for needle in needles:
        count = 0
        examples: List[Tuple[str, int, str]] = []
        seen_contexts = set()
        encoded = needle.encode("utf-16le")
        for segment_name in ("rodata", "data", "text"):
            blob = loaded[segment_name]
            base = by_name[segment_name].memory_offset
            encoded = needle.encode("utf-16le")
            start = 0
            while True:
                offset = blob.find(encoded, start)
                if offset < 0:
                    break
                count += 1
                context_offset, context = utf16le_ascii_context(blob, offset, len(encoded))
                if context not in seen_contexts:
                    example = (segment_name, base + context_offset, context)
                    if context == needle:
                        examples.insert(0, example)
                        if len(examples) > 6:
                            examples.pop()
                        seen_contexts.add(context)
                    elif len(examples) < 6:
                        examples.append(example)
                        seen_contexts.add(context)
                start = offset + 2
        result[needle] = (count, examples)
    return result


def arm64_xrefs(text: bytes, text_base: int, targets: set[int]) -> Dict[int, List[Tuple[int, str]]]:
    """Find direct ADR and nearby ADRP+ADD references to exact target addresses."""
    result: Dict[int, List[Tuple[int, str]]] = {target: [] for target in targets}
    recent_adrp: List[Tuple[int, int] | None] = [None] * 32  # (instruction index, page)

    usable = len(text) & ~3
    for byte_offset in range(0, usable, 4):
        insn = struct.unpack_from("<I", text, byte_offset)[0]
        pc = text_base + byte_offset
        instruction_index = byte_offset // 4

        if (insn & 0x9F000000) == 0x10000000:  # ADR
            immlo = (insn >> 29) & 0x3
            immhi = (insn >> 5) & 0x7FFFF
            immediate = sign_extend((immhi << 2) | immlo, 21)
            target = pc + immediate
            if target in targets:
                result[target].append((pc, "ADR"))

        if (insn & 0x9F000000) == 0x90000000:  # ADRP
            immlo = (insn >> 29) & 0x3
            immhi = (insn >> 5) & 0x7FFFF
            immediate = sign_extend((immhi << 2) | immlo, 21) << 12
            rd = insn & 0x1F
            recent_adrp[rd] = (instruction_index, (pc & ~0xFFF) + immediate)
            continue

        # ADD (immediate), 32- or 64-bit, with flags clear.
        if (insn & 0x7F000000) == 0x11000000:
            rn = (insn >> 5) & 0x1F
            prior = recent_adrp[rn]
            if prior is not None and instruction_index - prior[0] <= 8:
                shift = 12 if ((insn >> 22) & 1) else 0
                immediate = ((insn >> 10) & 0xFFF) << shift
                target = prior[1] + immediate
                if target in targets:
                    adrp_pc = text_base + prior[0] * 4
                    result[target].append((adrp_pc, f"ADRP+ADD (ADD at {pc:#x})"))

        # Expire stale candidates.  This deliberately avoids pretending to be
        # a full data-flow engine; Ghidra must confirm every reported XREF.
        if (instruction_index & 0xF) == 0:
            for register, prior in enumerate(recent_adrp):
                if prior is not None and instruction_index - prior[0] > 8:
                    recent_adrp[register] = None
    return result


def clipped(value: str, limit: int = 180) -> str:
    escaped = value.replace("\t", "\\t")
    if len(escaped) <= limit:
        return escaped
    return escaped[: limit - 3] + "..."


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("nso", type=Path)
    parser.add_argument("--max-strings", type=int, default=250)
    parser.add_argument("--max-xrefs", type=int, default=12)
    parser.add_argument("--no-xrefs", action="store_true")
    parser.add_argument(
        "--needles",
        nargs="*",
        default=[
            "StardewValley", "Stardew Valley", "Game1", "GameLocation",
            "Chest", "Furnace", "Copper Ore", "Copper Bar", "Coal",
            "Microsoft.Xna.Framework", "Mono", "mono_", "Xamarin",
            "UnityEngine", "il2cpp", "CoreCLR", "System.Private.CoreLib",
            "mscorlib", "SDL", "NintendoSDK",
        ],
    )
    args = parser.parse_args()

    file_data = args.nso.read_bytes()
    if len(file_data) < NSO_HEADER_SIZE:
        raise ValueError("file is smaller than an NSO header")
    header = file_data[:NSO_HEADER_SIZE]
    flags, module_id, module_name_location, segments, extents = parse_header(header)

    print(f"file={args.nso}")
    print(f"file_size={len(file_data):#x} ({len(file_data)})")
    print(f"file_sha256={hashlib.sha256(file_data).hexdigest().upper()}")
    print("magic=NSO0 VALID")
    print(f"flags={flags:#010x}")
    print(f"module_id_32={module_id.hex().upper()}")
    print(f"build_id_20={module_id[:20].hex().upper()}")
    print(f"header_module_name_file_range={module_name_location}")

    module_name_offset = u32(header, 0x1C)
    module_name_size = u32(header, 0x2C)
    module_name = file_data[module_name_offset:module_name_offset + module_name_size]
    print(
        "header_module_name_file_bytes="
        f"{module_name.rstrip(bytes([0])).decode('utf-8', errors='replace')!r}"
    )

    loaded: Dict[str, bytes] = {}
    print("segments:")
    for segment in segments:
        blob = load_segment(file_data, segment)
        loaded[segment.name] = blob
        actual_hash = hashlib.sha256(blob).digest()
        hash_status = "MATCH" if actual_hash == segment.expected_hash else "MISMATCH"
        print(
            f"  {segment.name}: file={segment.file_offset:#x} "
            f"stored={segment.compressed_size:#x} memory={segment.memory_offset:#x} "
            f"decompressed={segment.decompressed_size:#x} end={segment.memory_end:#x} "
            f"compressed={segment.compressed} sha256={actual_hash.hex().upper()} "
            f"header_hash={hash_status}"
        )

    # Horizon module images keep the loader-visible module path header at the
    # beginning of the decompressed rodata image. This is distinct from the two
    # legacy NSO header fields above, which elf2nso commonly leaves as 1:1.
    rodata = loaded["rodata"]
    rodata_module_name = "UNKNOWN"
    if len(rodata) >= 8:
        rodata_name_size = u32(rodata, 4)
        if rodata_name_size <= 0x200 and 8 + rodata_name_size <= len(rodata):
            rodata_module_name = rodata[8:8 + rodata_name_size].decode(
                "utf-8", errors="replace"
            )
    print(f"rodata_module_name={rodata_module_name!r}")

    bss_size = u32(header, 0x3C)
    bss_start = segments[2].memory_end
    print(f"  bss: memory={bss_start:#x} size={bss_size:#x} end={bss_start + bss_size:#x}")
    print("rodata_relative_extents:")
    for name, (offset, size) in extents.items():
        absolute = segments[1].memory_offset + offset
        print(f"  {name}: relative={offset:#x} size={size:#x} memory={absolute:#x}")

    api_offset, api_size = extents["api_info"]
    api_blob = loaded["rodata"][api_offset:api_offset + api_size]
    print("api_info_strings:")
    for _offset, value in iter_ascii_strings(api_blob, minimum=3):
        print(f"  {clipped(value)}")

    symbols = parse_dynsym(loaded["rodata"], extents)
    defined = [symbol for symbol in symbols if symbol[1] != 0]
    undefined = [symbol for symbol in symbols if symbol[1] == 0 and symbol[0]]
    print(
        f"dynsym_count={len(symbols)} defined={len(defined)} "
        f"undefined_named={len(undefined)}"
    )
    interesting_symbol_terms = (
        "mono", "il2cpp", "coreclr", "cxa", "unwind", "nnmain", "malloc",
        "free", "pthread", "socket", "nifm", "hid", "fs", "roinitialize",
    )
    print("interesting_dynsyms:")
    for name, section, value, size in symbols:
        if name and any(term in name.lower() for term in interesting_symbol_terms):
            state = "DEF" if section != 0 else "UND"
            print(f"  {state} value={value:#x} size={size:#x} {name}")
    print("defined_dynsyms:")
    for name, section, value, size in defined:
        print(f"  value={value:#x} size={size:#x} {name}")

    print("scanning ASCII strings...")
    candidates = find_candidate_strings(loaded, segments, args.needles)
    utf16_summary = summarize_utf16le_needles(loaded, segments, args.needles)
    utf16_examples = [
        (needle, segment_name, address, context)
        for needle, (_count, examples) in utf16_summary.items()
        for segment_name, address, context in examples
    ]
    targets = {item.address for item in candidates}
    targets.update(example[2] for example in utf16_examples)
    xrefs = (
        {} if args.no_xrefs
        else arm64_xrefs(loaded["text"], segments[0].memory_offset, targets)
    )
    print(f"candidate_string_count={len(candidates)}")
    print("utf16le_summary:")
    for needle, (count, examples) in utf16_summary.items():
        print(f"  needle={needle!r} count={count}")
        for segment_name, address, context in examples:
            references = xrefs.get(address, [])
            print(
                f"    {address:#010x} [{segment_name}] "
                f"xrefs={len(references)} context={clipped(context, 260)!r}"
            )
            for xref_address, kind in references[: args.max_xrefs]:
                print(f"      XREF {xref_address:#010x} {kind}")
    print(f"xref_target_count={len(targets)}")

    for index, item in enumerate(candidates[: args.max_strings]):
        references = xrefs.get(item.address, [])
        print(
            f"STRING {item.address:#010x} [{item.segment}] "
            f"needles={','.join(item.matched_needles)!r} xrefs={len(references)} "
            f"value={clipped(item.text)!r}"
        )
        for address, kind in references[: args.max_xrefs]:
            print(f"  XREF {address:#010x} {kind}")
        if len(references) > args.max_xrefs:
            print(f"  ... {len(references) - args.max_xrefs} more")

    if len(candidates) > args.max_strings:
        print(f"... {len(candidates) - args.max_strings} more candidate strings not displayed")
    print("xref_note=Heuristic only; confirm every XREF and function boundary in Ghidra.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(1)
