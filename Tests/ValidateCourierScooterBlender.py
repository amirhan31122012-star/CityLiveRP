"""Check the shipped FBX dimensions, slot names and the explicit material manifest."""
import bpy,json,hashlib,math
from pathlib import Path
root=Path(__file__).resolve().parents[1];source=root/'Assets/Vehicles/Courier'
manifest=json.loads((source/'manifest.json').read_text());file=source/'SM_CityScooter.fbx'
assert hashlib.sha256(file.read_bytes()).hexdigest()==manifest['mesh_sha256']
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(file))
meshes=[o for o in bpy.data.objects if o.type=='MESH'];assert len(meshes)==1
mesh=meshes[0];count=sum(len(p.vertices)-2 for p in mesh.data.polygons)
assert count==manifest['triangles'] and count<36000,count
assert all(abs(a-b)<1e-5 for a,b in zip(mesh.dimensions,manifest['dimensions_m']))
assert 1.85<mesh.dimensions.x<2.05 and .75<mesh.dimensions.y<.9 and 1.4<mesh.dimensions.z<1.5
assert all(abs(x-1)<1e-5 for x in mesh.scale)
assert set(m.name for m in mesh.data.materials)=={'Scooter_'+key for key in manifest['materials']}
for v in mesh.data.vertices:assert all(math.isfinite(c) for c in v.co)
for p in mesh.data.polygons:assert 0<=p.material_index<len(mesh.data.materials)
paint=manifest['materials']['Paint']['color'];assert paint[0]>paint[1]*10 and paint[0]>paint[2]*10
assert manifest['wheelbase_m']==1.31 and manifest['seat_height_m']==.72
(source/'Validation.json').write_text(json.dumps(dict(triangles=count,dimensions_m=list(mesh.dimensions),material_slots=len(mesh.data.materials),unit_scale=True),indent=2))
print('PASS: actual scooter FBX, dimensions, ten explicit colour materials, geometry budget, preserved seat/wheelbase')
