"""Offline exact-build metadata factory execution; never runs game logic.

Only memory allocation and the metadata constructor are allowed outside the
selected factory. Unknown calls fail instead of fabricating metadata. Output is
an analysis index; invokers and real consumers still establish runtime ABI.
"""
import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/tools/python'))
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_SP, UC_ARM64_REG_LR, UC_ARM64_REG_PC


class Metadata:
    def __init__(self):
        self.uc = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
        self.uc.mem_map(0, 0x10000000)
        header = (ROOT / 'exefs/main').read_bytes()[:256]
        assert header[0x40:0x54].hex() == 'a5c617c14a7f3f6620b3bc8136965a4822d32b9c'
        for name, base, hash_start in [('text', 0, 0xa0), ('rodata', 0x79b1000, 0xc0), ('data', 0xd38f000, 0xe0)]:
            data = (ROOT / ('analysis/main.' + name + '.bin')).read_bytes()
            assert hashlib.sha256(data).digest() == header[hash_start:hash_start+32], name
            self.uc.mem_write(base, data)
            if name == 'text': self.text = data
            if name == 'rodata': self.ro = data
        for offset, info, addend in struct.iter_unpack('<QQq', self.ro[0x68:0x17b2488]):
            if info == 0x403:
                self.uc.mem_write(offset, struct.pack('<Q', addend))
        self.uc.mem_map(0x20000000, 0x2000000)
        self.heap = 0x20000000
        self.uc.hook_add(UC_HOOK_CODE, self.hook)
        self.stop = 0x21fffff0

    def u(self, address, size=8):
        return int.from_bytes(self.uc.mem_read(address, size), 'little')

    def string(self, address):
        if not 0x79b1000 <= address < 0xd38f000: return None
        data = bytes(self.uc.mem_read(address, 2048))
        for n in range(0, len(data), 2):
            if data[n:n+2] == b'\0\0': return data[:n].decode('utf-16-le', errors='replace')
        return None

    def hook(self, uc, address, size, _):
        if address in (0x1020, 0x1030):
            length = uc.reg_read(UC_ARM64_REG_X0)
            assert 0 < length < 0x100000, ('allocation', length)
            result = self.heap
            self.heap += (length + 15) & ~15
            assert self.heap < 0x21000000
            uc.mem_write(result, bytes(length))
            uc.reg_write(UC_ARM64_REG_X0, result)
            uc.reg_write(UC_ARM64_REG_PC, uc.reg_read(UC_ARM64_REG_LR))
        elif not (self.active_start <= address < self.active_end or 0x27a0 <= address < 0x28e0):
            raise RuntimeError('Unapproved external code: ' + hex(address))

    def run(self, address):
        self.active_start = address
        end = address
        while struct.unpack_from('<I', self.text, end)[0] != 0xd65f03c0:
            end += 4
            assert end-address < 0x20000, 'factory has no bounded return'
        self.active_end = end + 4
        self.uc.reg_write(UC_ARM64_REG_SP, 0x21ff0000)
        self.uc.reg_write(UC_ARM64_REG_LR, self.stop)
        self.uc.emu_start(address, self.stop, count=200000)
        assert self.uc.reg_read(UC_ARM64_REG_PC) == self.stop, 'instruction limit'
        return self.uc.reg_read(UC_ARM64_REG_X0)

    def decode(self, factory):
        obj = self.run(factory)
        result = {'factory': hex(factory), 'name': self.string(self.u(obj+0x30)), 'size': self.u(obj), 'root': hex(self.u(obj+0x60)), 'base_type': hex(self.u(obj+0x88)), 'fields': [], 'methods': []}
        for key, pointer_offset, count_offset, stride in [('fields', 0xe8, 0xfa, 0x40), ('methods', 0x130, 0x140, 0x60)]:
            target, count = self.u(obj+pointer_offset), self.u(obj+count_offset, 2)
            if not target: continue
            entries = self.run(target)
            for i in range(count):
                at = entries + i * stride
                name = self.string(self.u(at))
                assert name is not None, (key, i, hex(at))
                entry = {'name': name}
                if key == 'fields':
                    entry.update(type=hex(self.u(at+8)), location=hex(self.u(at+0x10)), flags=hex(self.u(at+0x20, 4)))
                else:
                    entry.update(invoker=hex(self.u(at+0x10)), body=hex(self.u(at+0x18)), flags=hex(self.u(at+0x24, 4)), returns=hex(self.u(at+0x30)), args=[])
                    params, n = self.u(at+0x38), self.u(at+0x40, 1)
                    for j in range(n):
                        entry['args'].append({'name': self.string(self.u(params+j*0x18)), 'type': hex(self.u(params+j*0x18+8))})
                result[key].append(entry)
        return result


def main():
    p = argparse.ArgumentParser(__doc__)
    p.add_argument('factories', nargs='+', type=lambda s: int(s, 0))
    p.add_argument('--output', required=True)
    p.add_argument('--resolvers', action='store_true', help='Inputs are type resolvers, not metadata factories')
    args = p.parse_args()
    machine = Metadata()
    factories=args.factories
    if args.resolvers:
        factories=[]
        for resolver in args.factories:
            targets=[]
            for at in range(resolver,resolver+0x90,4):
                word=struct.unpack_from('<I',machine.text,at)[0]
                if word&0xfc000000==0x94000000:
                    immediate=word&0x3ffffff
                    if immediate&0x2000000: immediate-=0x4000000
                    target=at+immediate*4
                    if 0x1d00000<=target<0x7900000: targets.append(target)
            assert len(set(targets))==1, (hex(resolver),[hex(t) for t in targets])
            factories.append(targets[0])
    results = [machine.decode(a) for a in factories]
    Path(args.output).write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
    for result in results: print(result['name'], len(result['fields']), len(result['methods']))


if __name__ == '__main__': main()
