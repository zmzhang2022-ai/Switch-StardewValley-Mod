"""Compact read-only view of the offline metadata analysis index."""
import argparse
import json
import re
from pathlib import Path
p = argparse.ArgumentParser(__doc__)
p.add_argument('type')
p.add_argument('pattern', nargs='?', default='.')
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
seen=set()
for path in sorted((root/'analysis').glob('uiinfo-*.json')):
    content=json.loads(path.read_text(encoding='utf8'))
    if not isinstance(content,list): continue
    for t in content:
        if not isinstance(t,dict) or 'factory' not in t: continue
        if t['factory'] in seen: continue
        if t['name'].split('.')[-1] != a.type and a.type not in t['name']: continue
        seen.add(t['factory'])
        print(t['name'], 'factory', t['factory'], 'base', t.get('base_type'), 'root', t.get('root'))
        for key in ['fields','methods']:
            for f in t[key]:
                if re.search(a.pattern, f['name'], re.I): print(key, json.dumps(f, ensure_ascii=False))
