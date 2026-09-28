"""Generate/check exact-build fishing entry fingerprints; never executes game code."""
import argparse
import hashlib
import json
import re
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(__doc__)
p.add_argument('--generate', action='store_true')
a = p.parse_args()
main = (ROOT/'exefs/main').read_bytes()
assert hashlib.sha256(main).hexdigest() == '15c950769fa6a71e659549163066dacb36e838c8fd9cfd5e595f84d3f6b05dd0'
text = (ROOT/'analysis/main.text.bin').read_bytes()
assert hashlib.sha256(text).digest() == main[0xa0:0xc0]
source = (ROOT/'runtime/source/fishing/fishing.cpp').read_text(encoding='utf8')
offsets = {int(x, 16) for x in re.findall(r'Call<[^>]+>\((0x[0-9A-Fa-f]+)', source)}
offsets.update([0x51a2c0, 0x50bb40, 0x4fc340, 0x8ae3e0, 0x4f8e40,
                0x167fa90, 0x13257d8, 0x167ffb4, 0x1680d60, 0x1aa242c,
                0x14e9000, 0x1a95fc4, 0x1a95e80, 0x1a94cbc])
records = [{'offset': hex(off), 'words': list(struct.unpack_from('<4I', text, off))}
           for off in sorted(offsets)]
header = '#pragma once\n#include <cstdint>\nnamespace AutomateLite::Fishing {\n'
header += 'struct Signature { std::uintptr_t offset; std::uint32_t words[4]; };\n'
header += 'inline constexpr Signature kSignatures[]{\n'
header += ''.join('    {%s, {%s}},\n' % (r['offset'], ', '.join(hex(w)+'u' for w in r['words'])) for r in records)
header += '};\n}\n'
target = ROOT/'runtime/source/fishing/signatures.hpp'
if a.generate:
    target.write_text(header, encoding='utf8')
else:
    assert target.read_text(encoding='utf8') == header, 'Stale fishing fingerprints'
report = {'main_sha256': hashlib.sha256(main).hexdigest().upper(),
          'main_build_id': main[0x40:0x54].hex().upper(), 'entries': records,
          'passed': True, 'scope': 'Offline exact-main entry/consumer fingerprints; not runtime validation'}
(ROOT/'analysis/fishing-fingerprint-audit.json').write_text(json.dumps(report, indent=2), encoding='utf8')
print('Verified fishing fingerprints:', len(records))
