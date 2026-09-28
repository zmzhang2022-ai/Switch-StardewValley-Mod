"""Conservative static budget for this project's once-only trampoline installs.

Includes conditional trace hooks. This is not a C++ control-flow proof: callers
must still ensure installers are called once and never allocate in a loop.
"""
import argparse
import ast
from collections import Counter
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]

def constant(source, name, values):
    match = re.search(r'\b' + name + r'\s*=\s*([^;]+);', source)
    if not match:
        raise ValueError('Missing constant: ' + name)
    def resolve(node):
        if isinstance(node, ast.Constant) and type(node.value) is int:
            return node.value
        if isinstance(node, ast.Name):
            return values[node.id]
        if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Mult):
            return resolve(node.left) * resolve(node.right)
        raise ValueError('Unsupported constant expression: ' + name)
    return resolve(ast.parse(match.group(1).strip(), mode='eval').body)

def audit(jit_override=None):
    source = ROOT / 'runtime/source'
    layout = (source / 'lib/hook/nx64/pool_layout.hpp').read_text()
    values = {}
    for name in ('MaxInstructions', 'TrampolineWords', 'TrampolineBytes'):
        values[name] = constant(layout, name, values)
    jit = constant((source / 'program/setting.hpp').read_text(), 'JitSize', {})
    if jit_override is not None:
        jit = jit_override
    definitions, installs = [], []
    for path in sorted(source.rglob('*')):
        if path.suffix not in ('.cpp', '.hpp', '.h', '.c'):
            continue
        if path.relative_to(source).parts[0] in ('lib', 'nn', 'rtld'):
            continue
        code = re.sub(r'/\*.*?\*/|//[^\n]*', '', path.read_text(encoding='utf-8-sig'), flags=re.S)
        definitions += re.findall(r'HOOK_DEFINE_TRAMPOLINE\s*\(\s*(\w+)\s*\)', code)
        installs += re.findall(r'(\w+)::InstallAt(?:Offset|Ptr|Symbol)\s*\(', code)
        if re.search(r'nx64::Hook\s*\(', code):
            raise ValueError('Raw Hook call requires explicit budget review: ' + str(path))
    if not definitions or Counter(definitions) != Counter(installs) or any(n != 1 for n in Counter(definitions).values()):
        raise ValueError('Expected exactly one static installation per named trampoline; review hook inventory')
    capacity = jit // values['TrampolineBytes']
    spare = capacity - len(installs)
    return {'jit_bytes': jit, 'trampoline_bytes': values['TrampolineBytes'],
            'capacity': capacity, 'conservative_hooks_including_trace': len(installs),
            'minimum_spare_slots': 8, 'spare_slots': spare,
            'passed': spare >= 8, 'hooks': sorted(definitions),
            'limitation': 'Static inventory; installation must remain once-only and outside loops.'}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--jit-size', type=lambda v: int(v, 0), help='Counterfactual budget check only; does not edit source')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    result = audit(args.jit_size)
    encoded = json.dumps(result, indent=2)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(encoded + '\n', encoding='utf-8')
    print(encoded)
    return 0 if result['passed'] else 1

if __name__ == '__main__':
    raise SystemExit(main())
