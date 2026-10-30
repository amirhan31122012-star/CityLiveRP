"""Copy the pre-1.1 city, retain V1 rendering, remove parked cars and fix the sky.

Maps and shared textures/meshes are never overwritten. A successful restore
switches the startup marker to a new map referencing the existing city assets.
"""
import gc
import json
import os
from pathlib import Path
import re
import sys

import unreal

sys.path.insert(0, str(Path(__file__).parent))
from CityMobilePolicy import is_decorative_vehicle
from CityMobileSky import install_mobile_sky

MAP_PATTERN = r'/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity'
BASELINE_PATTERN = r'/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_(?:ModernCity|BeforeV11)'


def select_baseline(current, previous, exists):
    """Prefer the actual saved V1 map, never guess original lighting values."""
    if not re.fullmatch(MAP_PATTERN, current) or not exists(current):
        raise RuntimeError('Invalid or missing current city map')
    previous = previous if previous and previous.get('map') == current else {}
    if previous.get('graphics_baseline') == 'v1':
        baseline = previous.get('baseline_map', '')
        if not re.fullmatch(BASELINE_PATTERN, baseline) or not exists(baseline):
            raise RuntimeError('Original V1 map missing; send Saved/CityV11.json')
        return baseline
    backup = current.rsplit('/', 1)[0] + '/CityLive_BeforeV11'
    if exists(backup):
        return backup
    if previous:
        raise RuntimeError('Pre-1.1 backup missing; stop rather than reuse the optimized map')
    # Fresh import: no 1.1 update has changed this level yet.
    return current


def write_atomic(path, text):
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(text, encoding='utf-8')
    temporary.replace(path)


def main():
    project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
    token = os.environ['CITYLIVE_UPDATE_TOKEN']
    if not re.fullmatch(r'[0-9_]+', token):
        raise RuntimeError('Invalid update token')
    folder = project / 'Saved/CityUpdateV11' / token
    folder.mkdir(parents=True, exist_ok=True)
    marker = project / 'Saved/ModernCityMap.txt'
    current = marker.read_text(encoding='utf-8').strip()
    state_file = project / 'Saved/CityV11.json'
    previous = json.loads(state_file.read_text(encoding='utf-8')) if state_file.is_file() else None
    assets = unreal.EditorAssetLibrary
    baseline = select_baseline(current, previous, assets.does_asset_exist)
    destination = '/Game/CityLiveMaps/ModernCity/Run_' + token + '/CityLive_ModernCity'
    if assets.does_asset_exist(destination):
        raise RuntimeError('Restore destination already exists; run RestoreCityV1.cmd again')
    # Validate config before creating content or changing startup files.
    ini = project / 'Config/DefaultEngine.ini'
    config = ini.read_text(encoding='utf-8')
    for key in ('GameDefaultMap', 'EditorStartupMap'):
        pattern = r'^' + key + r'=[^\r\n]*'
        if len(re.findall(pattern, config, flags=re.MULTILINE)) != 1:
            raise RuntimeError('Missing or duplicate startup key: ' + key)
        config = re.sub(pattern, key + '=' + destination + '.CityLive_ModernCity',
                        config, flags=re.MULTILINE)
    unreal.log('CITYLIVE_V1_BASELINE ' + baseline + ' current preserved=' + current)
    copied = assets.duplicate_asset(baseline, destination)
    if not copied or not assets.save_loaded_asset(copied, only_if_is_dirty=False):
        raise RuntimeError('Cannot copy original V1 map; current city remains active')
    # DuplicateAsset returns a UWorld wrapper, not just an asset path. LoadLevel
    # unloads that package first and treats Python-held worlds as fatal leaks.
    # Drop the wrapper (including possible Python cycles) before switching maps.
    del copied
    gc.collect()
    levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if not levels.load_level(destination):
        raise RuntimeError('Cannot load restored city; current city remains active')
    removed = []
    for actor in list(actors.get_all_level_actors()):
        if not isinstance(actor, unreal.StaticMeshActor):
            continue
        mesh = actor.static_mesh_component.get_editor_property('static_mesh')
        name = actor.get_actor_label() + '_' + (mesh.get_name() if mesh else '')
        if is_decorative_vehicle(name):
            if not actors.destroy_actor(actor):
                raise RuntimeError('Cannot remove decorative vehicle ' + name)
            removed.append(name)
    # All other actors retain the saved V1 shadows, draw distances and lighting.
    root = destination.rsplit('/', 1)[0]
    install_mobile_sky(root, actors)
    for actor in actors.get_all_level_actors():
        if isinstance(actor, unreal.SkyLight):
            actor.light_component.recapture_sky()
    if not levels.save_current_level():
        raise RuntimeError('Cannot save restored city; current city remains active')
    summary = {'version': '1.1.2', 'graphics_baseline': 'v1', 'map': destination,
               'baseline_map': baseline, 'previous_map': current,
               'removed_vehicles': removed, 'mobile_sky': True}
    # Texture caps/streaming and shared geometry are left as in the first APK.
    # Failed work above cannot switch the active city or claim completion.
    write_atomic(state_file, json.dumps(summary, indent=2))
    write_atomic(ini, config)
    write_atomic(marker, destination)
    (folder / 'complete.ok').write_text(destination, encoding='utf-8')
    unreal.log('CITYLIVE_V1_RESTORED map=' + destination +
               ' removed vehicles=' + str(len(removed)) + ' graphics=v1 sky=fixed')


if __name__ == '__main__':
    main()
