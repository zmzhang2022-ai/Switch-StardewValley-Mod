"""Run vanilla ARM64 fade threshold branches with synthetic state, never callbacks."""
import hashlib
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'analysis/tools/python'))
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm64_const import UC_ARM64_REG_X20, UC_ARM64_REG_PC

main = (ROOT/'exefs/main').read_bytes()
text = (ROOT/'analysis/main.text.bin').read_bytes()
ro = (ROOT/'analysis/main.rodata.bin').read_bytes()
assert hashlib.sha256(main).hexdigest().upper() == '15C950769FA6A71E659549163066DACB36E838C8FD9CFD5E595F84D3F6B05DD0'
assert hashlib.sha256(text).digest() == main[0xa0:0xc0]
assert hashlib.sha256(ro).digest() == main[0xc0:0xe0]
robase = struct.unpack_from('<I',main,0x24)[0]
def constant(address): return struct.unpack_from('<f',ro,address-robase)[0]
upper,lower,speed = [constant(a) for a in (0xac76fc0,0xac77d7c,0xac77908)]
def f32(value): return struct.unpack('<f',struct.pack('<f',value))[0]

uc = Uc(UC_ARCH_ARM64,UC_MODE_ARM)
uc.mem_map(0x1136000,0x1000)
uc.mem_write(0x1136804,text[0x1136804:0x1136898])
uc.mem_map(0xac76000,0x2000)
uc.mem_write(0xac76000,ro[0xac76000-robase:0xac78000-robase])
uc.mem_map(0xdfef000,0x1000)
uc.mem_map(0x20000000,0x1000)
uc.mem_write(0xdfef600,struct.pack('<Q',0x20000800)) # messagePause = false
uc.reg_write(UC_ARM64_REG_X20,0x20000000)
exits={0x1136828:'black_callback',0x113687c:'clear_callback',0x113688c:'continue'}
def stop_before_callback(machine,address,size,data):
    if address in exits: machine.emu_stop()
uc.hook_add(UC_HOOK_CODE,stop_before_callback)
def classify(alpha):
    uc.mem_write(0x20000014,struct.pack('<f',alpha))
    uc.emu_start(0x1136804,0x1136898,count=40)
    return exits[uc.reg_read(UC_ARM64_REG_PC)]

boundaries=[]
for value,expected in [(1.0,'continue'),(upper,'continue'),(1.12,'black_callback'),
                       (0.0,'continue'),(lower,'continue'),(-0.12,'clear_callback')]:
    actual=classify(value)
    assert actual==expected,(value,actual,expected)
    boundaries.append(dict(alpha=value,result=actual))

def frames_until_callback(fade_in,elapsed,extra,legacy):
    alpha=0.0 if fade_in else 1.0
    target='black_callback' if fade_in else 'clear_callback'
    step=f32(speed*elapsed)
    for frame in range(1000):
        if classify(alpha)==target: return frame
        alpha=f32(alpha+(step if fade_in else -step)) # original update
        if extra:
            delta=f32(step*extra)
            alpha=f32(alpha+(delta if fade_in else -delta))
            if legacy: alpha=max(0.0,min(1.0,alpha))
    return None

cases=[]
for elapsed in (1,8,16,17,33,50,100):
    for extra in (0,1,2):
        for fade_in in (True,False):
            old=frames_until_callback(fade_in,elapsed,extra,True)
            fixed=frames_until_callback(fade_in,elapsed,extra,False)
            assert fixed is not None
            assert (old is None) == (extra>0)
            cases.append(dict(elapsed_ms=elapsed,multiplier=extra+1,fade_in=fade_in,
                              old_callback_frame=old,fixed_callback_frame=fixed))
report=dict(passed=True,original_branch='0x1136804..0x113688c',
            upper_threshold=upper,lower_threshold=lower,boundaries=boundaries,cases=cases,
            old_deadlocks=sum(c['old_callback_frame'] is None for c in cases),
            scope='Original ARM64 branch in isolated emulator, synthetic per-frame update ordering. No callbacks or whole game executed; hardware not tested.')
(ROOT/'analysis/fastanimations-fade-aot-test.json').write_text(json.dumps(report,indent=2),encoding='utf8')
print(json.dumps(dict(passed=True,boundary_cases=len(boundaries),timing_cases=len(cases),old_deadlocks=report['old_deadlocks'])))
