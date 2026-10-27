"""샘플 입력·몽타주·전투 데이터·캐릭터 BP·맵을 생성한다.

import_epic_content.ps1과 DrSampleEditor 빌드 후 DrFramework에서 실행:
UnrealEditor-Cmd DrSample.uproject -run=pythonscript
-script=Tools/build_sample_content.py -EnablePlugins=PythonScriptPlugin -nullrhi -unattended
"""
import unreal as u

BASE = '/Game/DrSample'
LYRA = '/Game/Characters/Heroes/Mannequin'
lib = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()

def load(path):
    asset = u.load_asset(path)
    if not asset:
        raise RuntimeError('필수 에셋 없음: ' + path)
    return asset

def save(asset):
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError('에셋 저장 실패: ' + asset.get_path_name())

def create(path, cls, factory):
    if lib.does_asset_exist(path):
        return load(path)
    folder, name = path.rsplit('/', 1)
    asset = tools.create_asset(name, folder, cls, factory)
    if not asset:
        raise RuntimeError('에셋 생성 실패: ' + path)
    return asset

def bp(name, parent):
    factory = u.BlueprintFactory()
    factory.set_editor_property('parent_class', parent)
    return create(BASE + '/Characters/' + name, u.Blueprint, factory)

def key(name):
    result = u.Key()
    result.import_text(name)
    return result

# 방향은 키 modifier로 만든다. 마우스와 패드 시점 입력은 별도 액션이다.
actions = {}
for name, value_type in [('Move', u.InputActionValueType.AXIS2D),
                         ('Look', u.InputActionValueType.AXIS2D),
                         ('GamepadLook', u.InputActionValueType.AXIS2D)] + [
                         (n, u.InputActionValueType.BOOLEAN) for n in
                         ['AttackLight', 'AttackHeavy', 'Dodge', 'LockOn', 'Sprint', 'Jump', 'Guard']]:
    action = create(BASE + '/Input/IA_DrSample_' + name, u.InputAction, u.InputAction_Factory())
    action.set_editor_property('value_type', value_type)
    save(action)
    actions[name] = action
context = create(BASE + '/Input/IMC_DrSample', u.InputMappingContext, u.InputMappingContext_Factory())
context.unmap_all()
for name, keys in {
    'Look': ['Mouse2D'], 'GamepadLook': ['Gamepad_Right2D'], 'Move': ['Gamepad_Left2D'],
    'AttackLight': ['LeftMouseButton', 'Gamepad_RightShoulder'],
    'AttackHeavy': ['RightMouseButton', 'Gamepad_RightTrigger'],
    'Dodge': ['LeftAlt', 'Gamepad_FaceButton_Right'],
    'LockOn': ['MiddleMouseButton', 'Gamepad_RightThumbstick'],
    'Sprint': ['LeftShift', 'Gamepad_LeftThumbstick'],
    'Jump': ['SpaceBar', 'Gamepad_FaceButton_Bottom'],
    'Guard': ['Q', 'Gamepad_LeftShoulder'],
}.items():
    for key_name in keys:
        context.map_key(actions[name], key(key_name))
for key_name, swizzle, negate in [('W', True, False), ('S', True, True), ('A', False, True), ('D', False, False)]:
    mapping = context.map_key(actions['Move'], key(key_name))
    modifiers = []
    if negate:
        modifiers.append(u.InputModifierNegate())
    if swizzle:
        modifier = u.InputModifierSwizzleAxis()
        modifier.set_editor_property('order', u.InputAxisSwizzle.YXZ)
        modifiers.append(modifier)
    mapping.set_editor_property('modifiers', modifiers)
save(context)

skeleton = load(LYRA + '/Meshes/SK_Mannequin')
montages = {}
for name, sequence in {
    'AttackLight': 'MM_Pistol_Melee', 'AttackHeavy': 'MM_Rifle_Melee',
    'HitFront': 'MM_HitReact_Front_Lgt_01', 'HitBack': 'MM_HitReact_Back_Lgt_01',
    'HitLeft': 'MM_HitReact_Left_Lgt_01', 'HitRight': 'MM_HitReact_Right_Lgt_01',
    'Death': 'MM_Death_Front_01',
}.items():
    source = load(LYRA + '/Animations/Actions/' + sequence)
    factory = u.AnimMontageFactory()
    factory.set_editor_property('target_skeleton', skeleton)
    factory.set_editor_property('source_animation', source)
    montage = create(BASE + '/Animations/AM_DrSample_' + name, u.AnimMontage, factory)
    save(montage)
    montages[name] = montage

factory = u.AnimBlueprintFactory()
factory.set_editor_property('target_skeleton', skeleton)
factory.set_editor_property('parent_class', u.DsSampleAnimInstance)
anim = create(BASE + '/Animations/ABP_DrSample', u.AnimBlueprint, factory)
if not u.DsSampleContent.build_animation_blueprint(anim, load(LYRA + '/Animations/Locomotion/Unarmed/BS_MM_Unarmed_Jog_Walk')):
    raise RuntimeError('이동 블렌드 → DefaultSlot 애님 그래프 생성 실패')
save(anim)

def data(name, cls):
    factory = u.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    return create(BASE + '/Combat/' + name, cls, factory)

combat = data('DA_DrSamplePlayerCombat', u.DsPlayerCombatData)
for field, montage_name in [('light_stages', 'AttackLight'), ('heavy_stages', 'AttackHeavy')]:
    stage = u.DsMeleeStage()
    stage.set_editor_property('montage', montages[montage_name])
    stage.set_editor_property('duration', montages[montage_name].get_play_length())
    stage.set_editor_property('active_start', .25)
    stage.set_editor_property('active_end', .45)
    stage.set_editor_property('lunge_distance', 25.0)
    stage.set_editor_property('poise_damage', 10.0)
    combat.set_editor_property(field, [stage])
dodge = combat.get_editor_property('dodge')
dodge.set_editor_property('roll_montages', [])
combat.set_editor_property('dodge', dodge)
save(combat)
boss_data = data('DA_DrSampleEnemy', u.DsBossData)
boss_data.set_editor_property('boss_name', 'Boss')
boss_data.set_editor_property('display_name', u.Text('훈련 상대 / Training Partner'))
for field, name in [('idle_anim', 'Idle'), ('move_anim', 'Move'), ('groggy_anim', 'Hit'), ('death_anim', 'Death')]:
    boss_data.set_editor_property(field, name)
stats = boss_data.get_editor_property('stats')
stats.set_editor_property('base_max_health', 150.0)
stats.set_editor_property('base_max_poise', 100.0)
boss_data.set_editor_property('stats', stats)
boss_data.set_editor_property('base_move_speed', 250.0)
save(boss_data)

player = bp('B_DrSamplePlayer', u.DsCharacter)
cdo = u.get_default_object(player.generated_class())
for component in cdo.get_components_by_class(u.SkeletalMeshComponent):
    if component.get_name() == 'MannyMesh':
        component.set_skeletal_mesh_asset(load(LYRA + '/Meshes/SKM_Manny'))
        component.set_anim_instance_class(anim.generated_class())
        component.set_editor_property('visibility_based_anim_tick_option', u.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
cdo.set_editor_property('input_mapping', context)
for field, action in [('move_action', 'Move'), ('look_action', 'Look'), ('gamepad_look_action', 'GamepadLook'), ('attack_light_action', 'AttackLight'), ('attack_heavy_action', 'AttackHeavy'), ('dodge_action', 'Dodge'), ('lock_on_action', 'LockOn'), ('sprint_action', 'Sprint'), ('jump_action', 'Jump'), ('guard_action', 'Guard')]:
    cdo.set_editor_property(field, actions[action])
for field, name in [('hit_react_front_montage', 'HitFront'), ('hit_react_back_montage', 'HitBack'), ('hit_react_left_montage', 'HitLeft'), ('hit_react_right_montage', 'HitRight'), ('death_montage', 'Death')]:
    cdo.set_editor_property(field, montages[name])
cdo.set_editor_property('default_abilities', [u.DsGameplayAbility_AttackLight, u.DsGameplayAbility_AttackHeavy, u.DsGameplayAbility_Dodge, u.DsGameplayAbility_Guard])
u.BlueprintEditorLibrary.compile_blueprint(player)
save(player)
enemy = bp('B_DrSampleEnemy', u.DsSampleEnemy)
cdo = u.get_default_object(enemy.generated_class())
mesh = cdo.get_component_by_class(u.SkeletalMeshComponent)
mesh.set_skeletal_mesh_asset(load(LYRA + '/Meshes/SKM_Quinn'))
mesh.set_anim_instance_class(anim.generated_class())
mesh.set_editor_property('visibility_based_anim_tick_option', u.VisibilityBasedAnimTickOption.ALWAYS_TICK_POSE_AND_REFRESH_BONES)
cdo.set_editor_property('boss_data', boss_data)
cdo.set_editor_property('hit_montage', montages['HitFront'])
for direction in ['Back', 'Left', 'Right']:
    cdo.set_editor_property('hit_' + direction.lower() + '_montage', montages['Hit' + direction])
u.BlueprintEditorLibrary.compile_blueprint(enemy)
save(enemy)
factory = u.BlueprintFactory()
factory.set_editor_property('parent_class', u.DsGameMode)
mode = create(BASE + '/B_DrSampleGameMode', u.Blueprint, factory)
u.get_default_object(mode.generated_class()).set_editor_property('default_pawn_class', player.generated_class())
u.BlueprintEditorLibrary.compile_blueprint(mode)
save(mode)

level = BASE + '/Maps/L_DrSample'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if not (levels.load_level(level) if lib.does_asset_exist(level) else levels.new_level(level)):
    raise RuntimeError('샘플 맵 생성 실패')
existing = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
def actor(cls, name, xyz, rotation=None):
    item = existing.get(name) or actors.spawn_actor_from_class(cls, u.Vector(*xyz), rotation or u.Rotator())
    item.set_actor_label(name)
    item.set_actor_location_and_rotation(u.Vector(*xyz), rotation or u.Rotator(), False, True)
    return item
floor = actor(u.StaticMeshActor, '샘플 바닥', (0, 0, -25))
floor.static_mesh_component.set_static_mesh(load('/Engine/BasicShapes/Cube'))
floor.static_mesh_component.set_material(0, load('/Engine/BasicShapes/BasicShapeMaterial'))
floor.set_actor_scale3d(u.Vector(2000, 2000, .5))
actor(u.PlayerStart, '플레이어 시작', (-400, 0, 100))
actor(enemy.generated_class(), '훈련 상대', (450, 0, 105), u.Rotator(yaw=180))
sun = actor(u.DirectionalLight, '태양', (0, 0, 500), u.Rotator(pitch=-45, yaw=-30))
sun.light_component.set_editor_property('intensity', 5.0)
actor(u.SkyLight, '하늘 조명', (0, 0, 500))
actor(u.SkyAtmosphere, '하늘', (0, 0, 0))
nav = actor(u.NavMeshBoundsVolume, '이동 영역', (0, 0, 100))
nav.set_actor_scale3d(u.Vector(20, 20, 3))
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', mode.generated_class())
if not levels.save_current_level():
    raise RuntimeError('샘플 맵 저장 실패')
u.log('DRSAMPLE_CONTENT_READY')
