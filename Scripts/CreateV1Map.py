"""Create the first driveable district in the local Unreal Editor (centimetres)."""
import os
from pathlib import Path
import unreal

DEST = '/Game/CityLiveMaps/CityLive_V1'
project = Path(unreal.Paths.project_dir())
# Never overwrite an edited map. Delete the map explicitly to regenerate it.
if unreal.EditorAssetLibrary.does_asset_exist(DEST):
    unreal.log('CityLive V1 already exists; keeping local edits.')
else:
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.new_level(DEST):
        raise RuntimeError('Cannot create CityLive V1 level')
    cube = unreal.load_asset('/Engine/BasicShapes/Cube.Cube')
    if not cube:
        raise RuntimeError('Engine cube asset missing')

    def material(name, color):
        path = '/Game/CityLiveMaps/Materials/' + name
        result = unreal.load_asset(path)
        if not result:
            result = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, '/Game/CityLiveMaps/Materials', unreal.Material, unreal.MaterialFactoryNew())
            node = unreal.MaterialEditingLibrary.create_material_expression(
                result, unreal.MaterialExpressionConstant3Vector)
            node.set_editor_property('constant', unreal.LinearColor(*color, 1))
            unreal.MaterialEditingLibrary.connect_material_property(node, '', unreal.MaterialProperty.MP_BASE_COLOR)
            unreal.MaterialEditingLibrary.recompile_material(result)
            if not unreal.EditorAssetLibrary.save_loaded_asset(result):
                raise RuntimeError('Cannot save ' + path)
        return result

    mats = {key: material('M_V1_' + key, value) for key, value in {
        'Grass': (.10, .20, .07), 'Asphalt': (.055, .06, .065),
        'Walk': (.34, .33, .30), 'Mark': (.85, .82, .65),
        'Brick': (.38, .22, .15), 'Plaster': (.58, .55, .43),
        'Roof': (.12, .14, .16), 'Window': (.07, .14, .20),
    }.items()}

    def box(label, x, y, z, sx, sy, sz, mat):
        actor = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z))
        actor.set_actor_label(label)
        component = actor.static_mesh_component
        component.set_static_mesh(cube)
        component.set_material(0, mats[mat])
        component.set_editor_property('mobility', unreal.ComponentMobility.STATIC)
        component.set_collision_profile_name('BlockAll')
        actor.set_actor_scale3d(unreal.Vector(sx / 100, sy / 100, sz / 100))
        return actor

    # A 360m district, road grid at -120m, 0, +120m. All roads share z=0.
    box('District ground', 0, 0, -60, 36000, 36000, 100, 'Grass')
    for axis in (-12000, 0, 12000):
        box('East west road', 0, axis, -15, 30000, 1200, 30, 'Asphalt')
        box('North south road', axis, 0, -15, 1200, 30000, 30, 'Asphalt')
        for sign in (-1, 1):
            box('Pavement', 0, axis + sign * 800, 5, 30000, 350, 40, 'Walk')
            box('Pavement', axis + sign * 800, 0, 5, 350, 30000, 40, 'Walk')
        for along in range(-14400, 14401, 600):
            if all(abs(along - crossing) > 1100 for crossing in (-12000, 0, 12000)):
                box('Lane dash', along, axis, 1, 240, 12, 2, 'Mark')
                box('Lane dash', axis, along, 1, 12, 240, 2, 'Mark')
    # Keep crossings flat and free of the pavement strips.
    for x in (-12000, 0, 12000):
        for y in (-12000, 0, 12000):
            # Remove intersecting long pavements by splitting them below instead.
            for lane in (-1, 1):
                for stripe in range(-4, 5):
                    box('Crosswalk', x + stripe * 100, y + lane * 1050, 2, 50, 260, 3, 'Mark')
    # Replace long pavements with segments to avoid raised barriers across roads.
    for actor in list(actors.get_all_level_actors()):
        if actor.get_actor_label().startswith('Pavement'):
            actors.destroy_actor(actor)
    for axis in (-12000, 0, 12000):
        for start, end in ((-15000, -13000), (-11000, -1000), (1000, 11000), (13000, 15000)):
            for sign in (-1, 1):
                box('Sidewalk segment', (start+end)/2, axis+sign*800, 5, end-start, 350, 40, 'Walk')
                box('Sidewalk segment', axis+sign*800, (start+end)/2, 5, 350, end-start, 40, 'Walk')
    for x in (-6500, 6500):
        for y in (-6500, 6500):
            box('Courtyard', x, y, -5, 8500, 8500, 10, 'Walk')
            for side in (-1, 1):
                bx = x + side*2800
                box('Apartment block', bx, y, 900, 1800, 6200, 1800, 'Brick' if side < 0 else 'Plaster')
                box('Roof', bx, y, 1830, 1900, 6300, 60, 'Roof')
                for floor in range(5):
                    for win in range(9):
                        for face in (-1, 1):
                            box('Window', bx + face*902, y-2600+win*650, 240+floor*330, 8, 180, 170, 'Window')
    # Flat parking apron beside spawn; native controller finds its sedan spot here.
    box('Spawn parking', 2800, -2300, -10, 4000, 2400, 20, 'Asphalt')
    for x in range(1500, 4501, 600):
        box('Parking stripe', x, -2300, 1, 10, 1800, 2, 'Mark')
    start = actors.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(1800, -2300, 110), unreal.Rotator(0, 0, 0))
    start.set_actor_label('CityLive spawn')
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings = world.get_world_settings()
    game_mode = unreal.load_class(None, '/Game/ThirdPerson/Blueprints/BP_ThirdPersonGameMode.BP_ThirdPersonGameMode_C')
    if not game_mode:
        raise RuntimeError('CityLive game mode missing')
    settings.set_editor_property('default_game_mode', game_mode)
    sun = actors.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 6000), unreal.Rotator(-45, -25, 0))
    sun.light_component.set_editor_property('mobility', unreal.ComponentMobility.MOVABLE)
    actors.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 5000))
    actors.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    actors.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
    if not levels.save_current_level():
        raise RuntimeError('Cannot save V1 map')

# Only change startup after a map has been saved successfully.
ini = project / 'Config/DefaultEngine.ini'
text = ini.read_text(encoding='utf-8')
import re
for key in ('GameDefaultMap', 'EditorStartupMap'):
    text = re.sub(r'^' + key + r'=.*$', key + '=' + DEST + '.CityLive_V1', text, flags=re.MULTILINE)
ini.write_text(text, encoding='utf-8')
marker = project / 'Saved/V1MapReady.ok'
marker.parent.mkdir(parents=True, exist_ok=True)
marker.write_text('CityLive RP V1 map saved', encoding='utf-8')
unreal.log('CITYLIVE_V1_MAP_READY')
