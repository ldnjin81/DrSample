"""DrFramework 에디터에서 Lyra 의존성 목록 산출 또는 복사본 노티파이 정리를 수행한다."""
import os
from pathlib import Path
import shutil
import unreal

ROOT = Path(unreal.Paths.project_dir()).resolve()
BASE = '/Game/Characters/Heroes/Mannequin/'
ROOTS = [BASE + p for p in [
    'Meshes/SKM_Manny', 'Meshes/SKM_Quinn', 'Meshes/SK_Mannequin', 'Rig/PA_Mannequin',
    'Animations/Locomotion/Unarmed/BS_MM_Unarmed_Jog_Walk',
    'Animations/Locomotion/Unarmed/MM_Unarmed_Idle_Ready',
    'Animations/Actions/MM_Pistol_Melee', 'Animations/Actions/MM_Rifle_Melee',
    'Animations/Actions/MM_HitReact_Front_Lgt_01',
    'Animations/Actions/MM_HitReact_Back_Lgt_01',
    'Animations/Actions/MM_HitReact_Left_Lgt_01',
    'Animations/Actions/MM_HitReact_Right_Lgt_01',
    'Animations/Actions/MM_Death_Front_01']]
registry = unreal.AssetRegistryHelpers.get_asset_registry()
manifest = ROOT / 'Tools/epic_content_manifest.txt'
discover = os.environ.get('DRSAMPLE_DISCOVER') == '1'
source = Path(os.environ.get('DRSAMPLE_LYRA_PATH', '')) / 'Content'

def filename(package):
    if not package.startswith('/Game/'):
        raise RuntimeError('지원하지 않는 Epic 패키지: ' + package)
    return package[len('/Game/'):] + '.uasset'

def copy_package(package):
    relative = filename(package)
    src, dst = source / relative, ROOT / 'Content' / relative
    if not src.is_file():
        raise RuntimeError('Lyra 의존성 파일 없음: ' + str(src))
    if not dst.exists():
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(src, dst)
    registry.scan_files_synchronous([str(dst)], True)

def clean(package):
    data = registry.get_asset_by_object_path(package + '.' + package.rsplit('/', 1)[-1])
    if str(data.asset_class_path.asset_name) == 'PhysicsAsset':
        asset = unreal.load_asset(package)
        unreal.DsSampleContent.clean_physics(asset)
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
            raise RuntimeError('물리 에셋 정리 저장 실패: ' + package)
        registry.scan_files_synchronous([str(ROOT / 'Content' / filename(package))], True)
        return
    if str(data.asset_class_path.asset_name) != 'AnimSequence':
        return
    asset = unreal.load_asset(package)
    if not asset:
        raise RuntimeError('애니메이션 로드 실패: ' + package)
    removed = unreal.DsSampleContent.clean_animation(asset)
    if removed:
        if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
            raise RuntimeError('애니메이션 정리 저장 실패: ' + package)
        registry.scan_files_synchronous([str(ROOT / 'Content' / filename(package))], True)
    unreal.log('DRSAMPLE_CLEAN {} {}'.format(package, removed))

if discover:
    pending, visited = list(ROOTS), set()
    options = unreal.AssetRegistryDependencyOptions(
        include_hard_package_references=True, include_soft_package_references=True)
    while pending:
        package = pending.pop()
        if package in visited:
            continue
        copy_package(package)
        clean(package)
        visited.add(package)
        for dep in registry.get_dependencies(package, options) or []:
            dep = str(dep)
            if dep == '/Script/LyraGame':
                raise RuntimeError('Lyra C++ 의존성이 남아 있음: ' + package)
            if dep.startswith('/Game/') and dep not in visited:
                pending.append(dep)
    manifest.write_text('# 에셋 레지스트리 의존성 폐쇄, Content 기준 상대 경로\n' +
                        '\n'.join(sorted(filename(p) for p in visited)) + '\n', encoding='utf-8')
    unreal.log('DRSAMPLE_MANIFEST_COUNT ' + str(len(visited)))
else:
    for line in manifest.read_text(encoding='utf-8-sig').splitlines():
        if line and not line.startswith('#'):
            package = '/Game/' + line.removesuffix('.uasset')
            registry.scan_files_synchronous([str(ROOT / 'Content' / line)], True)
            clean(package)
unreal.log('DRSAMPLE_EPIC_READY')
