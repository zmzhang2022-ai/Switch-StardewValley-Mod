"""Verify FastAnimations exact-build fingerprints and unchanged v13 consumers."""
import hashlib,json,re,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
source=ROOT/'runtime/source'
text=(ROOT/'analysis/main.text.bin').read_bytes()
main=(ROOT/'exefs/main').read_bytes()
assert hashlib.sha256(main).hexdigest().upper()=='15C950769FA6A71E659549163066DACB36E838C8FD9CFD5E595F84D3F6B05DD0'
assert hashlib.sha256(text).digest()==main[0xa0:0xc0]
header=(source/'fastanimations/signatures.hpp').read_text()
entries=re.findall(r'\{(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+)u\}',header)
assert len(entries)>40
for address,word in entries: assert struct.unpack_from('<I',text,int(address,16))[0]==int(word,16),address
baseline=json.loads((ROOT/'deploy/npc-map-locations-singleplayer-merged-v13/BUILD_INFO.json').read_text())
allowed={'hooks/game_update.cpp','project_metadata.hpp','uiinfo/settings.cpp','uiinfo/settings.hpp'}
changed=[]
for path,digest in baseline['source_sha256'].items():
    current=source/path
    assert current.is_file(),path
    if hashlib.sha256(current.read_bytes()).hexdigest().upper()!=digest:
        assert path in allowed,path
        changed.append(path)
assert not (source/'lookup').exists()
module=(source/'fastanimations/fastanimations.cpp').read_text()
assert 'InstallAtOffset' not in module
assert 'Call<void>(0x167FA90' not in module
report={'reference':'Fast Animations 1.16.0','baseline':'v13','fingerprints':len(entries),
        'changed_v13_files':changed,'unchanged_v13_files':len(baseline['source_sha256'])-len(changed),
        'extra_hooks':0,'hardware':'not executed'}
(ROOT/'analysis/fastanimations-static-audit.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
