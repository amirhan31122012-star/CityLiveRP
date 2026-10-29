"""Run in Blender: pruning unused None slots must preserve face assignments."""
from pathlib import Path
import sys
import bpy

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from CityMaterialIdentity import prune_unused_material_slots, bind_materials

bpy.ops.wm.read_factory_settings(use_empty=True)
mesh = bpy.data.meshes.new('paths')
mesh.from_pydata([(0,0,0), (1,0,0), (0,1,0), (1,1,0)], [], [(0,1,2), (1,3,2)])
obj = bpy.data.objects.new('Path and Imperfections', mesh)
bpy.context.collection.objects.link(obj)
a = bpy.data.materials.new('CL_Building_0000')
b = bpy.data.materials.new('CL_Building_0001')
for mat in (a,a,a,b,a):
    mesh.materials.append(mat)
for index in (0,2,4):
    obj.material_slots[index].material = None
mesh.polygons[0].material_index = 1
mesh.polygons[1].material_index = 3
assert len(mesh.materials) == 5 and mesh.materials[0] is None
prune_unused_material_slots([obj])
assert list(mesh.materials) == [a,b]
assert [p.material_index for p in mesh.polygons] == [0,1]
records = [{'slot': m.name} for m in (a,b)]
assert bind_materials([obj], records, bpy.data.materials) == records
assert len(mesh.polygons) == 2
print('UNUSED_EMPTY_SLOTS_TEST_OK: face materials preserved')
