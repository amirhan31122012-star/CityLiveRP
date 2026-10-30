"""Read all local city GLTFs, bake world placement and export separate FBX parts."""
import json
from pathlib import Path
import sys
import bpy
from mathutils import Matrix

sys.path.insert(0, str(Path(__file__).parent))
from CitySourceFiles import discover
from CityTextureChecks import ensure_textures_readable
from CityMaterialSource import material_sources
from CityMobilePolicy import is_decorative_vehicle
from CityMaterialIdentity import stage_scene, bind_materials, prune_unused_material_slots

source, output = map(Path, sys.argv[sys.argv.index('--') + 1:])
parts = discover(source)  # Validate every buffer/texture before processing anything.
output.mkdir(parents=True, exist_ok=True)
report = {'parts': [], 'skipped_parts': [], 'source': str(source), 'version': 2}
optional_geometry = {'Green Area', 'Towers'}
for part, path in parts:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'
    bpy.context.scene.unit_settings.scale_length = 1.0
    label = part.replace(' ', '')
    staged = stage_scene(path, output, label)
    bpy.ops.import_scene.gltf(filepath=str(staged))
    meshes = [o for o in bpy.context.scene.objects if o.type == 'MESH' and len(o.data.polygons)]
    meshes = [o for o in meshes if not is_decorative_vehicle(o.name)]
    if not meshes:
        if part not in optional_geometry:
            raise RuntimeError('No geometry in required city part: ' + str(path))
        report['skipped_parts'].append({'name': part, 'source': str(path),
                                        'reason': 'No mesh polygons in imported scene'})
        print('CITYLIVE_CITY_SKIP_EMPTY', part, str(path), flush=True)
        continue
    ensure_textures_readable(bpy.data.images)
    label = part.replace(' ', '')
    source_materials = material_sources(path, output, label)
    prune_unused_material_slots(meshes)
    records = bind_materials(meshes, source_materials, bpy.data.materials)
    if not records:
        raise RuntimeError('No original PBR materials found in ' + str(path))
    before = sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes)
    for obj in meshes:
        # Unique mesh data and absolute vertices: every part shares the same origin.
        matrix = obj.matrix_world.copy()
        obj.data = obj.data.copy()
        obj.data.transform(matrix)
        if matrix.determinant() < 0:
            obj.data.flip_normals()
        obj.parent = None
        obj.matrix_world = Matrix.Identity(4)
        if part == 'Road' and 'traffic' in obj.name.lower():
            bpy.context.view_layer.objects.active = obj
            bpy.ops.object.select_all(action='DESELECT')
            obj.select_set(True)
            mod = obj.modifiers.new('Android traffic simplification', 'DECIMATE')
            mod.ratio = .15
            bpy.ops.object.modifier_apply(modifier=mod.name)
    # Export into run-specific Saved folder; originals never overwritten.
    bpy.ops.object.select_all(action='DESELECT')
    for obj in meshes:
        obj.select_set(True)
    label = part.replace(' ', '')
    filename = output / (label + '.fbx')
    bpy.ops.export_scene.fbx(filepath=str(filename), use_selection=True, object_types={'MESH'},
                             path_mode='COPY', embed_textures=True, add_leaf_bones=False,
                             axis_forward='-Y', axis_up='Z', apply_unit_scale=True,
                             bake_anim=False, use_mesh_modifiers=True)
    after = sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in meshes)
    report['parts'].append({'name': label, 'fbx': str(filename), 'triangles_before': before,
                             'triangles_after': after, 'objects': len(meshes), 'materials': records})
    print('CITYLIVE_CITY_PART', label, before, after, flush=True)
(output / 'manifest.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('CITYLIVE_CITY_PREPARED', flush=True)
