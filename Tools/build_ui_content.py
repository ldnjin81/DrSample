"""UI 콘텐츠를 만든다: 일시정지 입력(IA_Pause·IMC_Menu·IMC_UI, DefaultGame.ini DrUIKitSettings가 가리킴)과
타이틀 맵 L_Title(게임 모드 ADsTitleGameMode: 플레이어 없이 시네 카메라 + 타이틀 메뉴, 패키지 기본 맵).

build_sample_content.py 다음에 실행(이미 있으면 다시 저장만 한다):
UnrealEditor-Cmd DrSample.uproject -run=pythonscript
-script=Tools/build_ui_content.py -EnablePlugins=PythonScriptPlugin -nullrhi -unattended
"""
import unreal as u

lib = u.EditorAssetLibrary
tools = u.AssetToolsHelpers.get_asset_tools()

def create(path, cls, factory):
    if lib.does_asset_exist(path):
        return u.load_asset(path)
    folder, name = path.rsplit('/', 1)
    asset = tools.create_asset(name, folder, cls, factory)
    if not asset:
        raise RuntimeError('에셋 생성 실패: ' + path)
    return asset

def save(asset):
    if not lib.save_loaded_asset(asset, False):
        raise RuntimeError('에셋 저장 실패: ' + asset.get_path_name())

def key(name):
    result = u.Key()
    result.import_text(name)
    return result

# 일시정지: 게임 중에는 IMC_Menu(우선순위 100)가 IA_Pause를 받는다. 메뉴가 열리면 메뉴 위젯이 Escape·Start를
# 직접 뒤로가기로 처리하므로 IMC_UI(우선순위 200)는 게임 입력만 가리는 빈 컨텍스트다.
pause = create('/Game/DrSample/Input/IA_Pause', u.InputAction, u.InputAction_Factory())
pause.set_editor_property('value_type', u.InputActionValueType.BOOLEAN)
save(pause)
menu = create('/Game/DrSample/Input/IMC_Menu', u.InputMappingContext, u.InputMappingContext_Factory())
menu.unmap_all()
for name in ['Escape', 'Gamepad_Special_Right']:
    menu.map_key(pause, key(name))
save(menu)
save(create('/Game/DrSample/Input/IMC_UI', u.InputMappingContext, u.InputMappingContext_Factory()))

level = '/Game/DrSample/Maps/L_Title'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if not (levels.load_level(level) if lib.does_asset_exist(level) else levels.new_level(level)):
    raise RuntimeError('타이틀 맵 생성 실패')
existing = {a.get_actor_label(): a for a in actors.get_all_level_actors()}
def actor(cls, name, xyz, rotation=None):
    item = existing.get(name) or actors.spawn_actor_from_class(cls, u.Vector(*xyz), rotation or u.Rotator())
    item.set_actor_label(name)
    item.set_actor_location_and_rotation(u.Vector(*xyz), rotation or u.Rotator(), False, True)
    return item
# 타이틀 카메라(30900, -750, 1100)가 내려다보는 곳에 바닥을 둔다.
floor = actor(u.StaticMeshActor, '타이틀 바닥', (28000, -1000, 0))
floor.static_mesh_component.set_static_mesh(u.load_asset('/Engine/BasicShapes/Cube'))
floor.static_mesh_component.set_material(0, u.load_asset('/Engine/BasicShapes/BasicShapeMaterial'))
floor.set_actor_scale3d(u.Vector(200, 200, .5))
sun = actor(u.DirectionalLight, '태양', (0, 0, 500), u.Rotator(pitch=-35, yaw=-30))
sun.light_component.set_editor_property('intensity', 5.0)
actor(u.SkyLight, '하늘 조명', (0, 0, 500))
actor(u.SkyAtmosphere, '하늘', (0, 0, 0))
world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode', u.DsTitleGameMode)
if not levels.save_current_level():
    raise RuntimeError('타이틀 맵 저장 실패')
u.log('DRSAMPLE_UI_READY')
