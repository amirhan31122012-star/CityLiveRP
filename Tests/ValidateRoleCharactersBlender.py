"""Validate exported FBX files, not only generator objects. Run in Blender."""
import bpy,json,math,hashlib
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=root/'Assets/Characters/Roles'
manifest=json.loads((source/'manifest.json').read_text())
if 'asset_hashes' in manifest:
 for name,digest in manifest['asset_hashes'].items():assert hashlib.sha256((source/name).read_bytes()).hexdigest()==digest,name
def load(path):
 bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(path))
 return next(o for o in bpy.data.objects if o.type=='ARMATURE'),next(o for o in bpy.data.objects if o.type=='MESH')
arm,mesh=load(root/'Assets/Characters/Beige/SK_CityBeige.fbx')
ref={b.name:b.matrix_local.copy() for b in arm.data.bones}
results=[]
for name in ('Civilian','Police','Medic','Courier','CourierPack'):
 arm,mesh=load(source/('SK_City'+name+'.fbx'))
 assert all(abs(v-1)<1e-5 for v in arm.scale) and all(abs(v-1)<1e-5 for v in mesh.scale)
 assert set(b.name for b in arm.data.bones)==set(ref)
 for b in arm.data.bones:
  assert max(abs(b.matrix_local[r][c]-ref[b.name][r][c]) for r in range(4) for c in range(4))<1e-4,b.name
 for v in mesh.data.vertices:
  assert v.groups and len(v.groups)<=4,(name,v.index)
  assert abs(sum(g.weight for g in v.groups)-1)<.002,(name,v.index)
  assert all(math.isfinite(x) for x in v.co)
 triangles=sum(len(p.vertices)-2 for p in mesh.data.polygons)
 assert triangles<50000,(name,triangles)
 assert mesh.data.uv_layers or name=='CourierPack'
 if name!='CourierPack':
  assert all(-1e-5<=axis<=1.00001 for loop in mesh.data.uv_layers.active.data for axis in loop.uv),(name,'UV outside repacked atlas')
  texture=bpy.data.images.load(str(source/('T_Role_'+name+'.png')))
  assert tuple(texture.size)==(2048,2048),(name,texture.size)
  for prefix in ('T_Face_','T_FaceRough_','T_RoleNormal_','T_RoleSurface_'):
   detail=bpy.data.images.load(str(source/(prefix+name+'.png')))
   assert tuple(detail.size)==(1024,1024),(name,prefix,detail.size)
  face_slots={i for i,m in enumerate(mesh.data.materials) if m and m.name=='Role_Face_'+name}
  assert face_slots,(name,'face material missing')
  face_triangles=sum(len(p.vertices)-2 for p in mesh.data.polygons if p.material_index in face_slots)
  assert face_triangles>10000,(name,'face detail budget lost',face_triangles)
  assert any(m and m.name=='Role_Eyes' for m in mesh.data.materials),(name,'3D eyes missing')
 else:face_triangles=0
 results.append(dict(role=name,triangles=triangles,face_triangles=face_triangles,bones=len(ref),max_influences=4,scale=[1,1,1]))
 print('VALIDATED_ROLE',name,triangles,flush=True)
(source/'Validation.json').write_text(json.dumps(results,indent=2))
