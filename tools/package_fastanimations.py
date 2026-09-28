"""Package the verified v13-based FastAnimations candidate; never overwrite a delivery."""
import hashlib,json,re,shutil,zipfile
from pathlib import Path
from analyze_nso import parse_header,load_segment
ROOT=Path(__file__).resolve().parents[1]
NAME='fast-animations-singleplayer-merged-v13-1-1'
EXPECTED='8F43C7ECDFB4055D2EAC3B49B0F22C11A1D771D0754B24F7CC81FD80610C4415'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def read(p): return p.read_text(encoding='utf-8-sig')
def inventory(root): return {p.relative_to(root).as_posix():p for p in root.rglob('*') if p.is_file()}
def installs(files):
    return sorted((k,a,re.sub(r'\s+','',b)) for k,p in files.items() if p.suffix=='.cpp'
        for a,b in re.findall(r'(\w+)::InstallAtOffset\(([^;]+)\);',read(p)))
def main():
    out=ROOT/'deploy'/NAME; archive=out.with_suffix('.zip')
    assert not out.exists() and not archive.exists(),'Refusing to overwrite a delivery'
    baseline=ROOT/'deploy/npc-map-locations-singleplayer-merged-v13'
    prior=json.loads(read(baseline/'BUILD_INFO.json'))
    current=inventory(ROOT/'runtime/source'); old=inventory(baseline/'source/runtime/source')
    assert all(sha(p)==prior['source_sha256'][k] for k,p in old.items())
    added=sorted(set(current)-set(old)); removed=sorted(set(old)-set(current))
    changed=sorted(k for k in old if k in current and sha(old[k])!=sha(current[k]))
    assert not removed
    assert changed==['hooks/game_update.cpp','project_metadata.hpp','uiinfo/settings.cpp','uiinfo/settings.hpp']
    assert added==['fastanimations/fastanimations.cpp','fastanimations/fastanimations.hpp','fastanimations/logic.hpp','fastanimations/signatures.hpp']
    assert installs(current)==installs(old)
    budget=json.loads(read(ROOT/'analysis/fastanimations-hook-budget.json'))
    assert budget['passed'] and budget['conservative_hooks_including_trace']==32 and budget['spare_slots']==8
    hooks=json.loads(read(ROOT/'analysis/v13-intermod-conflict-audit.json'))['hooks']
    addresses=[int(h['offset'],16) for h in hooks]
    assert len(addresses)==32 and all(abs(a-b)>=16 for i,a in enumerate(addresses) for b in addresses[i+1:])
    sigs=[int(x,16) for x in re.findall(r'\{(0x[0-9A-Fa-f]+),',read(current['fastanimations/signatures.hpp']))]
    assert len(sigs)==59 and not any(abs(a-b)<16 for a in sigs for b in addresses)
    static=json.loads(read(ROOT/'analysis/fastanimations-static-audit.json'))
    assert static['fingerprints']==59 and static['unchanged_v13_files']==253
    fade=json.loads(read(ROOT/'analysis/fastanimations-fade-aot-test.json'))
    assert fade['passed'] and fade['old_deadlocks']==28 and len(fade['cases'])==42
    failed=ROOT/'deploy/fast-animations-singleplayer-merged-v13-1'
    failed_info=json.loads(read(failed/'BUILD_INFO.json'))
    assert sha(failed/'atmosphere/contents/0100E65002BB8000/exefs/subsdk9')=='EB390AAE9F260E2A9CDD4A633F1D81DA6FE6746460E435BDF17D3F26C91C15BE'
    assert set(current)==set(failed_info['source_sha256'])
    fix_changed=sorted(k for k,p in current.items() if sha(p)!=failed_info['source_sha256'][k])
    assert fix_changed==['fastanimations/fastanimations.cpp','fastanimations/logic.hpp','project_metadata.hpp']
    build=ROOT/'runtime/build-clang'; stage=ROOT/'atmosphere/contents/0100E65002BB8000/exefs'
    nso=build/'subsdk9-normal'; elf=build/'automate_lite-normal.elf'
    assert sha(nso)==sha(build/'subsdk9')==sha(stage/'subsdk9')==EXPECTED
    assert sha(build/'main.npdm')==sha(stage/'main.npdm')=='F63D42112A3866CF6BF04ABD011F30BB3DF5E852BE244A2200FDDCD9A4898D85'
    assert sha(ROOT/'exefs/main')=='15C950769FA6A71E659549163066DACB36E838C8FD9CFD5E595F84D3F6B05DD0'
    assert sha(ROOT/'exefs/main.npdm')=='38DB701EC4BA0A688A96F413238FCCBB751D0CEACC8D12D0AB2F077E07562CCE'
    assert all(p.stat().st_mtime<=elf.stat().st_mtime for p in current.values())
    blob=nso.read_bytes(); _,module,_,segments,_=parse_header(blob[:256])
    for segment in segments:
        data=load_segment(blob,segment)
        assert len(data)==segment.decompressed_size and segment.hash_enabled and hashlib.sha256(data).digest()==segment.expected_hash
    audit=json.loads(read(ROOT/'analysis/fastanimations-nso-audit.json'))
    assert audit['candidate']['valid'] and audit['candidate']['sha256']==EXPECTED
    assert audit['comparison']['different_bytes_in_overlap']==0
    assert read(ROOT/'analysis/fastanimations-imports.txt').split()==read(ROOT/'analysis/npcmap-v13-imports.txt').split()
    symbols=read(ROOT/'analysis/fastanimations-symbols.txt'); assembly=read(ROOT/'analysis/fastanimations-elf.asm')
    for name in ['FastAnimations::Update()','AutoFishingStartCastingHook::Callback(void*)','NpcMapHook::Callback(void*, void*, float)']:
        assert name in symbols,name
    assert 'AutomateLite::Lookup::' not in symbols
    assert re.search(r'\b0000000000002000\s+t\s+exl::hook::nx64::impl::s_HookJit::s_Area',symbols)
    assert re.search(r'cmp\s+w8, #0x27\s*\n[^\n]+b.hi',assembly)
    assert re.search(r'mov\s+w9, #0xc8',assembly)
    log=read(ROOT/'analysis/fastanimations-build.log')
    assert 'error:' not in log and all(s in log for s in ['Fatal proof: False','Phase 8 trace: False','Phase 8 audit: False'])
    assert 'FastAnimations config migration/timing, v13 source, NPC map, fishing and UI static checks passed.' in read(ROOT/'analysis/fastanimations-tests.log')
    conflict=dict(static_passed=True,unchanged_hook_installations=True,hook_count=32,hook_address_overlaps=[],
        fingerprint_install_order_conflicts=[],unchanged_v13_sources=253,unchanged_sdk_imports=True,
        fishing_source_unchanged=True,lookup_removed=True,hardware='not executed',
        limitation='Shared original object updates and pre-existing v13 risks require hardware regression; not proof of runtime isolation')
    (ROOT/'analysis/fastanimations-conflict-audit.json').write_text(json.dumps(conflict,indent=2))
    info=dict(name=NAME,game_version='1.6.15.3',main_build_id='A5C617C14A7F3F6620B3BC8136965A4822D32B9C',
        subsdk9_sha256=EXPECTED,main_npdm_sha256=sha(stage/'main.npdm'),module_id=module[:20].hex().upper(),elf_sha256=sha(elf),
        baseline=baseline.name,baseline_subsdk9_sha256=prior['subsdk9_sha256'],
        reference_mod=dict(name='Fast Animations',version='1.16.0',author='Pathoschild',dll_sha256=sha(ROOT/'PCmod/FastAnimations/FastAnimations.dll')),
        scope=dict(singleplayer=True,default_multiplier=2,selectable_multipliers=[2,3],settings_toggle=True,
            skip_eat_confirmation_when_enabled=True,weapons=True,slingshot=True,cutscenes=False,lookup=False,
            auto_fishing_chord='L3+R3',independent_cosmetic_effect_parity=False),
        diagnostics=dict(FatalProof=False,Phase8Trace=False,Phase8Audit=False),
        validation=dict(static_tests='passed',arm64_build='passed',nso_segments='passed',hook_pool=budget,conflict_audit=conflict,fade_aot=fade,hardware='not_run'),
        fix=dict(supersedes='v13.1',reported_failure='Black screen after leaving house',changed_source_files=fix_changed,
            reason='Remove [0,1] clamp that prevents vanilla alpha > 1.1 and alpha < -0.1 completion callbacks'),
        changed_existing_source_files=changed,new_source_files=added,removed_source_files=removed,unchanged_source_files=253,
        source_sha256={k:sha(p) for k,p in sorted(current.items())},warnings=sorted(set(re.findall(r'warning: ([^\n]+)',log))))
    target=out/'atmosphere/contents/0100E65002BB8000/exefs'; target.mkdir(parents=True)
    for name in ['subsdk9','main.npdm']: shutil.copy2(stage/name,target/name)
    (out/'symbols').mkdir(); shutil.copy2(elf,out/'symbols/fastanimations-v13-1-1.elf')
    shutil.copy2(ROOT/'runtime/LICENSE',out/'LICENSE')
    shutil.copy2(ROOT/'docs/FAST_ANIMATIONS_SWITCH.md',out/'完整设计与验证.md')
    (out/'安装说明.txt').write_text('FastAnimations v13.1.1 · Switch 1.6.15.3 单人整合包\n\n'
        '修复 v13.1 出门黑屏：移除黑幕状态值的错误限制，保留原版切图与淡出完成回调。\n'
        '基于 v13；旧 Lookup v14 已作废。本包不含 Lookup。\n'
        '完全退出游戏，将 atmosphere 文件夹合并至 SD 卡根目录，成对覆盖 subsdk9/main.npdm。\n'
        '保留自动化、电梯、四戒指、UI信息、自动钓鱼和 NPC 大地图。\n'
        '默认动画 2 倍，包含武器/弹弓，跳过手动吃喝确认；剧情过场原速。\n'
        '原 UI 信息设置页末尾可关闭加速，或开启 3 倍。返回键保存。\n'
        'L3+R3 仍为自动钓鱼；无新增快捷键或小地图。\n\n'
        '静态检查、本地构建、NSO三段校验通过；真机启动、画面、性能及功能组合回归未执行。\n'
        '部分独立粒子保持原速，完整边界见完整设计与验证.md。\n'
        '回退时成对恢复保留的 v13 subsdk9/main.npdm。\n'
        'source、symbols、audit 为源码及证据，不需要复制到 SD 卡。\n',encoding='utf8')
    audit_dir=out/'audit'; audit_dir.mkdir()
    for pattern in ['fastanimations-*','uiinfo-fastanimations-*.json']:
        for p in (ROOT/'analysis').glob(pattern):
            if p.is_file(): shutil.copy2(p,audit_dir/p.name)
    shutil.copytree(ROOT/'analysis/fastanimations-pc-il',audit_dir/'pc-reference-il')
    for name in ['npcmap-v13-imports.txt','v13-intermod-conflict-audit.json']:
        shutil.copy2(ROOT/'analysis'/name,audit_dir/name)
    src=out/'source'
    for origin,dest in [(ROOT/'runtime/source',src/'runtime/source'),(ROOT/'runtime/misc',src/'runtime/misc'),
                        (ROOT/'toolchains/compat',src/'toolchains/compat')]: shutil.copytree(origin,dest)
    (src/'tests').mkdir(); (src/'docs').mkdir(); (src/'tools').mkdir()
    for p in (ROOT/'tests').glob('*.cpp'):
        if not p.name.startswith('lookup'): shutil.copy2(p,src/'tests'/p.name)
    for name in ['FAST_ANIMATIONS_SWITCH.md','AUTO_FISHING_SWITCH.md','NPC_MAP_LOCATIONS_SWITCH.md','V13_CONFLICT_REVIEW.md','UIINFO_SUITE2_SWITCH.md']:
        shutil.copy2(ROOT/'docs'/name,src/'docs'/name)
    for name in ['build_poc_clang.ps1','patch_npdm_syscalls.py','analyze_nso.py','check_hook_budget.py',
        'test_fastanimations.ps1','test_fastanimations_fade_aot.py','audit_fastanimations.py','package_fastanimations.py','test_fishing.ps1','audit_fishing.py',
        'test_uiinfo.ps1','test_npcmap.ps1','audit_npcmap.py','extract_aot_metadata.py','inspect_uiinfo_aot.py',
        'query_uiinfo_metadata.py','dump_dotnet_il.py']: shutil.copy2(ROOT/'tools'/name,src/'tools'/name)
    for name in ['LICENSE','config.mk','Makefile']: shutil.copy2(ROOT/'runtime'/name,src/'runtime'/name)
    for name in ['AGENTS.md','PROJECT_LESSONS.md']: shutil.copy2(ROOT/name,src/name)
    (out/'BUILD_INFO.json').write_text(json.dumps(info,ensure_ascii=False,indent=2),encoding='utf8')
    with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for p in sorted(out.rglob('*')):
            if p.is_file(): z.write(p,p.relative_to(out).as_posix())
    with zipfile.ZipFile(archive) as z:
        assert z.testzip() is None
        for name in ['subsdk9','main.npdm']:
            assert z.read('atmosphere/contents/0100E65002BB8000/exefs/'+name)==(stage/name).read_bytes()
        for name,digest in info['source_sha256'].items():
            assert hashlib.sha256(z.read('source/runtime/source/'+name)).hexdigest().upper()==digest
    print(json.dumps({'zip':str(archive),'zip_sha256':sha(archive),'subsdk9_sha256':EXPECTED,'module_id':info['module_id']},indent=2))
if __name__=='__main__': main()
