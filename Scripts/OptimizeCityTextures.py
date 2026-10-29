"""Reduce active imported city texture memory without repeating geometry import."""
import json
import os
from pathlib import Path
import re
import sys
import unreal

sys.path.insert(0, str(Path(__file__).parent))
from CityTextureBudget import apply_city_texture_budget

project = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
marker = project / 'Saved/ModernCityMap.txt'
if not marker.is_file():
    raise RuntimeError('Imported city map marker missing; finish ImportModernCity.cmd first')
map_path = marker.read_text(encoding='utf-8').strip()
if not re.fullmatch(r'/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity', map_path):
    raise RuntimeError('Invalid city map marker')
if not unreal.EditorAssetLibrary.does_asset_exist(map_path):
    raise RuntimeError('Imported city map no longer exists: ' + map_path)
root = map_path.rsplit('/', 1)[0]
token = os.environ['CITYLIVE_TEXTURE_TOKEN']
if not re.fullmatch(r'[0-9_]+', token):
    raise RuntimeError('Invalid optimization run token')
folder = project / 'Saved/CityTextureBudget' / token
folder.mkdir(parents=True, exist_ok=True)
changes = []
for path in unreal.EditorAssetLibrary.list_assets(root, recursive=True):
    # Avoid loading all buildings or textures from abandoned older map imports.
    name = path.rsplit('/', 1)[-1].split('.', 1)[0]
    role = next((r for r in ('Color', 'Normal', 'Data') if name.startswith('T_' + r + '_')), None)
    if role is None:
        continue
    texture = unreal.load_asset(path)
    if not isinstance(texture, unreal.Texture2D):
        continue
    was_streamed = not texture.get_editor_property('never_stream')
    previous, limit = apply_city_texture_budget(texture, role)
    changes.append({'asset': path, 'previous_limit': previous, 'limit': limit,
                    'previous_streaming_enabled': was_streamed})
    # Preserve colour space, normal-map compression and green channel settings.
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
        raise RuntimeError('Cannot save city texture: ' + path)
    unreal.log('CITYLIVE_TEXTURE_BUDGET ' + name + ' max=' + str(limit))
if not changes:
    raise RuntimeError('No directly imported city PBR textures found')
(folder / 'changes.json').write_text(json.dumps({'map': map_path, 'textures': changes}, indent=2), encoding='utf-8')
(folder / 'complete.ok').write_text(str(len(changes)), encoding='utf-8')
unreal.log('CITYLIVE_TEXTURE_BUDGET_COMPLETE count=' + str(len(changes)))
