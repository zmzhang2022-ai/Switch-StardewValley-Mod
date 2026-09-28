"""Read-only queries against the project's cached, exact-build AOT segments."""
import argparse
import hashlib
import struct
from pathlib import Path

import capstone

ROOT = Path(__file__).resolve().parents[1]


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument('--disasm', nargs=2, action='append', default=[])
    p.add_argument('--got', nargs='*', default=[])
    p.add_argument('--names', nargs='*', default=[])
    p.add_argument('--calls', nargs='*', default=[])
    p.add_argument('--loads', nargs='*', default=[])
    p.add_argument('--range', nargs=2, default=['0x727c000', '0x728c000'])
    args = p.parse_args()
    text = (ROOT / 'analysis/main.text.bin').read_bytes()
    ro = (ROOT / 'analysis/main.rodata.bin').read_bytes()
    header = (ROOT / 'exefs/main').open('rb').read(256)
    assert header[0x40:0x54].hex() == 'a5c617c14a7f3f6620b3bc8136965a4822d32b9c', 'Wrong main build'
    assert hashlib.sha256(text).digest() == header[0xa0:0xc0], 'Stale text cache'
    assert hashlib.sha256(ro).digest() == header[0xc0:0xe0], 'Stale rodata cache'
    cs = capstone.Cs(capstone.CS_ARCH_ARM64, capstone.CS_MODE_ARM)
    for start, end in args.disasm:
        start, end = int(start, 0), int(end, 0)
        print('RANGE', hex(start), hex(end))
        for ins in cs.disasm(text[start:end], start):
            print(hex(ins.address), ins.mnemonic, ins.op_str)
    wanted = {int(a, 0) for a in args.got}
    if wanted:
        # DT_RELA/DT_RELASZ in this main's MOD0 dynamic table.
        for offset, info, addend in struct.iter_unpack('<QQq', ro[0x68:0x17b2488]):
            if offset in wanted:
                print('RELA', hex(offset), hex(info), hex(addend))
    if args.calls:
        destinations = {int(a, 0) for a in args.calls}
        start, end = [int(a, 0) for a in args.range]
        for off in range(start, end, 4):
            word = struct.unpack_from('<I', text, off)[0]
            if word & 0x7c000000 == 0x14000000:
                imm = word & 0x3ffffff
                if imm & 0x2000000:
                    imm -= 0x4000000
                if off + imm * 4 in destinations:
                    print('BRANCH', hex(off), hex(off + imm * 4))
    if args.loads:
        destinations = {int(a, 0) for a in args.loads}
        start, end = [int(a, 0) for a in args.range]
        pages = {}
        for off in range(start, end, 4):
            word = struct.unpack_from('<I', text, off)[0]
            if word & 0x9f000000 == 0x90000000:
                imm = ((word >> 29) & 3) | (((word >> 5) & 0x7ffff) << 2)
                if imm & 0x100000:
                    imm -= 0x200000
                pages[word & 31] = (off, (off & ~4095) + imm * 4096)
            elif word & 0xffc00000 == 0xf9400000:
                previous = pages.get((word >> 5) & 31)
                if previous and off - previous[0] < 256:
                    address = previous[1] + ((word >> 10) & 4095) * 8
                    if address in destinations:
                        print('LOAD-CANDIDATE', hex(off), hex(address))
    if args.names:
        from analyze_nso import arm64_xrefs
        targets = {}
        for name in args.names:
            pos = 0
            needle = name.encode('utf-16-le') + b'\0\0'
            while True:
                pos = ro.find(needle, pos)
                if pos < 0:
                    break
                targets[0x79b1000 + pos] = name
                pos += 2
        start, end = [int(a, 0) for a in args.range]
        for address, hits in arm64_xrefs(text[start:end], start, targets).items():
            if hits:
                print(targets[address], hex(address), [(hex(a), kind) for a, kind in hits])


if __name__ == '__main__':
    main()
