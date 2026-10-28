"""Blender: build a separate four-door rig from the user's exported sedan.

Run: blender -b --python PrepareSedanDoorsBlender.py -- input.fbx output.fbx
Does not contain or distribute the licensed car geometry.
"""
import bpy
import sys
import json
from pathlib import Path
from collections import defaultdict
from mathutils import Vector

args = sys.argv[sys.argv.index('--') + 1:]
source, destination = map(Path, args[:2])
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(source.resolve()))
mesh_object = next(o for o in bpy.data.objects if o.type == 'MESH')
armature = next(o for o in bpy.data.objects if o.type == 'ARMATURE')
mesh = mesh_object.data
original_materials = [m.name for m in mesh.materials]
original_vertex_count = len(mesh.vertices)
original_polygon_count = len(mesh.polygons)

# Build islands across coincident hard-normal/UV splits for classification only.
# The actual topology, UVs, normals and material slots are not welded or modified.
parents = list(range(len(mesh.vertices)))
def root(index):
    while parents[index] != index:
        parents[index] = parents[parents[index]]
        index = parents[index]
    return index
def union(a, b):
    a, b = root(a), root(b)
    if a != b:
        parents[b] = a
for edge in mesh.edges:
    union(*edge.vertices)
coincident = {}
for vertex in mesh.vertices:
    key = tuple(round(c, 3) for c in vertex.co) + tuple(g.group for g in vertex.groups)
    if key in coincident:
        union(vertex.index, coincident[key])
    else:
        coincident[key] = vertex.index
islands = defaultdict(list)
for vertex in mesh.vertices:
    islands[root(vertex.index)].append(vertex)
island_materials = defaultdict(set)
for polygon in mesh.polygons:
    island_materials[root(polygon.vertices[0])].add(polygon.material_index)

door_vertices = defaultdict(list)
for vertices in islands.values():
    # Never steal wheels or steering wheel vertices from the original rig.
    if any(v.groups for v in vertices):
        continue
    low = [min(v.co[a] for v in vertices) for a in range(3)]
    high = [max(v.co[a] for v in vertices) for a in range(3)]
    if low[2] < 30 or high[2] > 137 or low[0] < -130.6 or high[0] > 94:
        continue
    side = 'Left' if low[1] > 60 else 'Right' if high[1] < -60 else None
    if not side:
        continue
    # The central interior B pillar remains fixed, unlike the door's trim.
    if low[0] < -24 and high[0] < 10 and low[2] < 45 and high[2] > 90:
        continue
    # Dashboard-side vents/trim are near the A pillar, but are not part of the door.
    material_indices = island_materials[root(vertices[0].index)]
    inner_y = min(abs(low[1]), abs(high[1]))
    if material_indices == {3} and low[0] > 45 and inner_y < 70 and high[2] < 94:
        continue
    if low[0] >= -38.5 and high[0] > -20:
        door = 'Door_Front_' + side
    elif high[0] <= -23:
        door = 'Door_Rear_' + side
    else:
        continue
    door_vertices[door].extend(v.index for v in vertices)

expected_doors = ['Door_Front_Left', 'Door_Front_Right', 'Door_Rear_Left', 'Door_Rear_Right']
for name in expected_doors:
    if len(door_vertices[name]) < 1500:
        raise RuntimeError('Door selection incomplete: ' + name)

# Normalize the source export's centimetre parent scale into metre coordinates.
for obj in (mesh_object, armature):
    world = obj.matrix_world.copy()
    obj.parent = None
    obj.matrix_world = world
bpy.ops.object.select_all(action='DESELECT')
mesh_object.select_set(True)
armature.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
armature.name = 'Armature'
mesh_object.name = 'SK_CitySedan_4Doors'
bpy.ops.object.mode_set(mode='EDIT')
root_bone = armature.data.edit_bones.new('Vehicle_ROOT')
root_bone.head = (0, 0, 0)
root_bone.tail = (0, 0, .1)
for bone in armature.data.edit_bones:
    if bone != root_bone and bone.parent is None:
        bone.parent = root_bone
for name in expected_doors:
    bone = armature.data.edit_bones.new(name)
    x = .84 if 'Front' in name else -.26
    y = .91 if 'Left' in name else -.91
    bone.head = (x, y, .58)
    bone.tail = (x, y, .68)
    bone.parent = root_bone
bpy.ops.object.mode_set(mode='OBJECT')

root_group = mesh_object.vertex_groups.new(name='Vehicle_ROOT')
unweighted = [v.index for v in mesh.vertices if not v.groups]
root_group.add(unweighted, 1.0, 'REPLACE')
for name, indices in door_vertices.items():
    root_group.remove(indices)
    group = mesh_object.vertex_groups.new(name=name)
    group.add(indices, 1.0, 'REPLACE')

# Preserve world coordinates and bind to the new explicit skeleton root.
world = mesh_object.matrix_world.copy()
mesh_object.parent = armature
mesh_object.matrix_world = world
for modifier in mesh_object.modifiers:
    if modifier.type == 'ARMATURE':
        modifier.object = armature

assert len(mesh.vertices) == original_vertex_count
assert len(mesh.polygons) == original_polygon_count
assert [m.name for m in mesh.materials] == original_materials
assert all(abs(sum(g.weight for g in v.groups) - 1.0) < 0.0001 for v in mesh.vertices)
destination.parent.mkdir(parents=True, exist_ok=True)
if destination.exists():
    raise RuntimeError('Refusing to overwrite existing file: ' + str(destination))
bpy.ops.wm.save_as_mainfile(filepath=str(destination.with_suffix('.blend')))
bpy.ops.object.select_all(action='DESELECT')
mesh_object.select_set(True)
armature.select_set(True)
bpy.context.view_layer.objects.active = armature
bpy.ops.export_scene.fbx(filepath=str(destination), use_selection=True, object_types={'MESH', 'ARMATURE'},
    add_leaf_bones=False, bake_anim=False, use_mesh_modifiers=False,
    axis_forward='-Y', axis_up='Z', apply_unit_scale=True)
report = {'vertices': original_vertex_count, 'polygons': original_polygon_count,
          'materials': original_materials,
          'doors': {name: len(door_vertices[name]) for name in expected_doors},
          'bones': [b.name for b in armature.data.bones]}
destination.with_suffix('.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print('CITYLIVE_DOOR_RIG', json.dumps(report))
