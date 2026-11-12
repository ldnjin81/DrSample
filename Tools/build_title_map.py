"""타이틀 맵(L_Title)을 만든다. 게임 모드는 ADsTitleGameMode(플레이어 없이 시네 카메라 + 타이틀 메뉴).

build_sample_content.py 다음에 실행(이미 있으면 다시 저장만 한다):
UnrealEditor-Cmd DrSample.uproject -run=pythonscript
-script=Tools/build_title_map.py -EnablePlugins=PythonScriptPlugin -nullrhi -unattended
"""
import unreal as u

level = '/Game/DrSample/Maps/L_Title'
levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
actors = u.get_editor_subsystem(u.EditorActorSubsystem)
if not (levels.load_level(level) if u.EditorAssetLibrary.does_asset_exist(level) else levels.new_level(level)):
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
u.log('DRSAMPLE_TITLE_READY')
