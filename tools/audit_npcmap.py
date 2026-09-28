"""Verify exact-build NPC map entry/consumer fingerprints (offline)."""
import argparse, hashlib, json, re, struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(__doc__)
p.add_argument('--generate',action='store_true')
a=p.parse_args()
main=(ROOT/'exefs/main').read_bytes()
assert hashlib.sha256(main).hexdigest()=='15c950769fa6a71e659549163066dacb36e838c8fd9cfd5e595f84d3f6b05dd0'
text=(ROOT/'analysis/main.text.bin').read_bytes()
assert hashlib.sha256(text).digest()==main[0xa0:0xc0]
source=(ROOT/'runtime/source/uiinfo/npc_map.cpp').read_text(encoding='utf8')
offsets={int(s,16) for s in re.findall(r'Call<[^>]+>\((0x[\da-fA-F]+)',source)}
# Virtual invokers, map transform/scale consumer, NetBool and friendship helpers.
offsets.update([0x173c7f0,0x173dd40,0x173c64c,0x173d2dc,0x173ca00,
 0x74a96e8,0x74ad3d8,0x71da5c0,0x71d9c58,0x190fab0,0x1192790,
 0x10e3770,0xde8220,0xaf0,0x1188600])
entries=[dict(offset=hex(x),words=list(struct.unpack_from('<4I',text,x))) for x in sorted(offsets)]
header='#pragma once\n#include <cstdint>\nnamespace AutomateLite::UIInfo::NpcMapSignatures {\n'
header+='struct Signature { std::uintptr_t offset; std::uint32_t words[4]; };\ninline constexpr Signature Entries[]{\n'
header+=''.join('    {%s, {%s}},\n'%(e['offset'],', '.join(hex(w)+'u' for w in e['words'])) for e in entries)
header+='};\n}\n'
target=ROOT/'runtime/source/uiinfo/npc_map_signatures.hpp'
if a.generate: target.write_text(header,encoding='utf8')
else: assert target.read_text(encoding='utf8')==header,'Stale NPC map fingerprints'
(ROOT/'analysis/npcmap-fingerprint-audit.json').write_text(json.dumps(dict(passed=True,entries=entries,
 main_sha256=hashlib.sha256(main).hexdigest().upper(),scope='Offline exact-main entries/consumers, not hardware execution'),indent=2),encoding='utf8')
print('NPC map fingerprints verified:',len(entries))
