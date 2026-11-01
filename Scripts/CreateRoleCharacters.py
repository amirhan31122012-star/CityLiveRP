"""Build four rigged role variants from the credited CC BY character.
Run Blender -b -P Scripts/CreateRoleCharacters.py. No downloaded game assets.
"""
import bpy,bmesh,math,json,sys,argparse,hashlib,numpy as np
from pathlib import Path
from mathutils import Vector
from mathutils.bvhtree import BVHTree
from mathutils.kdtree import KDTree
root=Path(__file__).resolve().parents[1];out=root/'Assets/Characters/Roles'
sys.path.insert(0,str(root/'Scripts'))
from CityRoleTexture import bake
from CityRoleTailoring import add as tailor
from CityRoleFace import shape,bake_face,make_eyes,eyes_material,skin_tone
out.mkdir(parents=True,exist_ok=True)
records=[]
parser=argparse.ArgumentParser();parser.add_argument('--role',choices=('Civilian','Police','Medic','Courier'))
args=parser.parse_args(sys.argv[sys.argv.index('--')+1:] if '--' in sys.argv else [])
for role in ([args.role] if args.role else ('Civilian','Police','Medic','Courier')):
 bpy.ops.wm.read_factory_settings(use_empty=True)
 bpy.ops.import_scene.fbx(filepath=str(root/'Assets/Characters/Beige/SK_CityBeige.fbx'))
 body=next(o for o in bpy.data.objects if o.type=='MESH');arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
 bpy.context.view_layer.objects.active=body
 for modifier in list(body.modifiers):
  if modifier.type=='ARMATURE':body.modifiers.remove(modifier)
 # Separate new, bounded meshes rather than altering the original rig/asset.
 # Give the face its own geometry budget instead of collapsing it like trousers.
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_mode(type='FACE');bpy.ops.mesh.select_all(action='DESELECT')
 bm=bmesh.from_edit_mesh(body.data)
 for face in bm.faces:
  if all(v.co.z>1.595 for v in face.verts):face.select_set(True)
 bmesh.update_edit_mesh(body.data)
 bpy.ops.mesh.separate(type='SELECTED');bpy.ops.object.mode_set(mode='OBJECT')
 head=next(o for o in bpy.context.selected_objects if o.type=='MESH' and o!=body)
 for part,ratio in ((body,.145),(head,.67)):
  bpy.context.view_layer.objects.active=part
  dec=part.modifiers.new('Mobile geometry','DECIMATE');dec.ratio=ratio;dec.use_collapse_triangulate=True
  bpy.ops.object.modifier_apply(modifier=dec.name)
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);head.select_set(True);bpy.context.view_layer.objects.active=body;bpy.ops.object.join()
 bm=bmesh.new();bm.from_mesh(body.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.000005);bm.to_mesh(body.data);bm.free()
 for v in body.data.vertices:
  v.co=shape(v.co,role)
 original=body.data.materials[0];original.name='Role_Skin'
 # Use only the existing photographed skin texture; clothing gets its own slots.
 original.use_nodes=True;nodes=original.node_tree.nodes;nodes.clear()
 bs=nodes.new('ShaderNodeBsdfPrincipled');bs.inputs['Roughness'].default_value=.64
 tex=nodes.new('ShaderNodeTexImage');tex.image=bpy.data.images.load(str(root/'Assets/Characters/Beige/Image_1.png'),check_existing=True)
 output=nodes.new('ShaderNodeOutputMaterial');original.node_tree.links.new(tex.outputs['Color'],bs.inputs['Base Color']);original.node_tree.links.new(bs.outputs[0],output.inputs[0])
 palette={'Dark':(.005,.007,.011,1),'White':(.72,.75,.75,1),'Navy':(.008,.018,.04,1),'Teal':(.025,.27,.29,1),'Red':(.48,.025,.04,1),'Steel':(.22,.25,.28,1),'Gold':(.5,.32,.085,1),'Hair':(.004,.003,.002,1),'Ginger':(.22,.075,.023,1),'GrayHair':(.14,.14,.13,1),'Beard':(.045,.035,.028,1)}
 mats={'Skin':original}
 for name,color in palette.items():
  m=bpy.data.materials.new('Role_'+name);m.diffuse_color=color;m.use_nodes=True
  p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=color;p.inputs['Roughness'].default_value=.95 if name in ('Hair','Ginger','GrayHair','Beard') else .68
  if name in ('Hair','Ginger','GrayHair','Beard'):p.inputs['Specular IOR Level'].default_value=0
  if name in ('Steel','Gold'):p.inputs['Metallic'].default_value=.7;p.inputs['Roughness'].default_value=.35
  mats[name]=m;body.data.materials.append(m)
 original_uv=np.asarray([loop.uv[:] for loop in body.data.uv_layers.active.data])
 # The scan repeats UV islands over multiple tiles. Repack before painting so
 # different garments cannot overwrite each other's pixels in the shared tile.
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);bpy.context.view_layer.objects.active=body
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT')
 bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.008)
 bpy.ops.object.mode_set(mode='OBJECT')
 texture=bake(body,role,palette,root/'Assets/Characters/Beige/Image_1.png',out/('T_Role_'+role+'.png'),original_uv,skin_tone=skin_tone(root,role))
 original.name='Role_'+role;tex.image=texture
 normal_tex=nodes.new('ShaderNodeTexImage');normal_tex.image=bpy.data.images.load(str(out/('T_RoleNormal_'+role+'.png')));normal_tex.image.colorspace_settings.name='Non-Color'
 normal=nodes.new('ShaderNodeNormalMap');original.node_tree.links.new(normal_tex.outputs['Color'],normal.inputs['Color']);original.node_tree.links.new(normal.outputs[0],bs.inputs['Normal'])
 mask=nodes.new('ShaderNodeTexImage');mask.image=bpy.data.images.load(str(out/('T_RoleSurface_'+role+'.png')));mask.image.colorspace_settings.name='Non-Color'
 separate=nodes.new('ShaderNodeSeparateRGB');original.node_tree.links.new(mask.outputs['Color'],separate.inputs[0]);original.node_tree.links.new(separate.outputs['R'],bs.inputs['Roughness']);original.node_tree.links.new(separate.outputs['G'],bs.inputs['Specular IOR Level'])
 for p in body.data.polygons:p.material_index=0;p.use_smooth=True
 # A separate 1K face atlas gives eyebrows, lips and skin their own texel budget.
 face_indices=set()
 for p in body.data.polygons:
  centre=sum((body.data.vertices[i].co for i in p.vertices),Vector())/len(p.vertices)
  if all(body.data.vertices[i].co.z>1.595 for i in p.vertices) and (centre.z>1.67 or abs(centre.x)<.074):face_indices.add(p.index)
 bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_mode(type='FACE');bpy.ops.mesh.select_all(action='DESELECT')
 bm=bmesh.from_edit_mesh(body.data);bm.faces.ensure_lookup_table()
 for index in face_indices:bm.faces[index].select_set(True)
 bmesh.update_edit_mesh(body.data)
 bpy.ops.uv.smart_project(angle_limit=math.radians(66),island_margin=.012)
 bpy.ops.object.mode_set(mode='OBJECT')
 bake_face(body,role,root,original_uv,face_indices)
 coords=[v.co.copy() for v in body.data.vertices]
 bvh=BVHTree.FromPolygons(coords,[list(p.vertices) for p in body.data.polygons])
 kd=KDTree(len(coords))
 for i,p in enumerate(coords):kd.insert(p,i)
 kd.balance();parts=[body]
 make_eyes(body,arm,role,eyes_material(root),parts)
 def weights(o,bone=None):
  for name in [b.name for b in arm.data.bones]:
   if o.vertex_groups.get(name) is None:o.vertex_groups.new(name=name)
  for v in o.data.vertices:
   if bone:items=[(bone,1)]
   else:
    _,index,_=kd.find(o.matrix_world@v.co)
    items=[(body.vertex_groups[g.group].name,g.weight) for g in body.data.vertices[index].groups]
   total=sum(w for n,w in items) or 1
   for n,w in items:o.vertex_groups[n].add([v.index],w/total,'REPLACE')
 def finish(o,material,bone=None):
  o.data.materials.clear();o.data.materials.append(mats[material]);weights(o,bone);parts.append(o)
  return o
 def box(name,pos,size,material,bone=None,bevel=.008):
  bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
  mod=o.modifiers.new('Tailored edges','BEVEL');mod.width=bevel;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
  return finish(o,material,bone)
 def line(name,points,width,material,bone=None):
  c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=width;c.bevel_resolution=2
  sp=c.splines.new('POLY');sp.points.add(len(points)-1)
  for p,co in zip(sp.points,points):p.co=(*co,1)
  o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);bpy.context.view_layer.objects.active=o;o.select_set(True)
  bpy.ops.object.convert(target='MESH');o=bpy.context.object
  return finish(o,material,bone)
 def surface(x,z,front=True):
  hit,_,_,_=bvh.ray_cast(Vector((x,-1 if front else 1,z)),Vector((0,1 if front else -1,0)),2)
  return tuple(hit+Vector((0,-.004 if front else .004,0))) if hit else (x,-.15 if front else .12,z)
 def patch(name,points,mat,bone=None):
  mesh=bpy.data.meshes.new(name);mesh.from_pydata(points,[],[list(range(len(points)))]);o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o);finish(o,mat,bone);return o
 if role in ('Civilian','Courier'):
  line('Zip',[surface(0,z) for z in (1.0,1.1,1.2,1.3,1.4,1.5)],.002,'Steel')
  for side in (-1,1):box('Cargo pocket',surface(side*.17,.67),(.085,.028,.14),'Dark','thigh_l' if side>0 else 'thigh_r',.012)
 if role=='Civilian':
  line('Shoulder strap',[surface(-.14,1.5),surface(-.06,1.34),surface(.05,1.17),(.14,-.23,1.06)],.018,'Dark')
  box('Crossbody bag',(.11,-.235,1.075),(.25,.055,.19),'Dark','spine_01',.025)
  line('Crest',[(.09,-.266,1.08),(.095,-.266,1.105),(.11,-.266,1.09),(.125,-.266,1.115),(.135,-.266,1.08),(.09,-.266,1.08)],.0018,'White','spine_01')
  # Original angular dragon/flame graphic, projected onto the back fabric.
  for x in (-.05,0,.05):
   line('Back dragon',[surface(x+.08,1.47,False),surface(x-.04,1.4,False),surface(x+.04,1.31,False),surface(x-.07,1.22,False)],.007,'White')
 if role=='Police':
  for side in (-1,1):
   patch('Collar',[surface(side*.02,1.51),surface(side*.1,1.49),surface(side*.055,1.42)],'Navy')
   for z in (1.29,1.37):
    x=side*.09;patch('Shirt pocket',[surface(x-.038,z),surface(x+.038,z),surface(x+.038,z-.07),surface(x-.038,z-.07)],'Navy')
   top,_,_,_=bvh.ray_cast(Vector((side*.205,-.025,2)),Vector((0,0,-1)),1)
   box('Shoulder board',(side*.205,-.025,(top.z if top else 1.58)+.006),(.085,.055,.012),'Navy',None,.002)
  for z in (1.05,1.15,1.25,1.35,1.44):box('Uniform button',surface(0,z),(.008,.005,.008),'Steel',None,.002)
  bpy.ops.mesh.primitive_torus_add(major_radius=.155,minor_radius=.013,major_segments=32,minor_segments=6,location=(0,-.01,.95))
  o=bpy.context.object;o.scale.y=.66;bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);finish(o,'Dark','pelvis')
  box('Belt buckle',(0,-.12,.95),(.045,.012,.033),'Steel','pelvis')
  box('Radio',(.18,-.01,1.04),(.045,.05,.12),'Dark','pelvis')
  patch('Chest badge',[surface(.11,1.4),surface(.13,1.37),surface(.11,1.345),surface(.09,1.37)],'Gold')
 if role=='Medic':
  for side in (-1,1):
   x=side*.035;z=1.745
   points=[(x+.029*math.cos(a),-.251,z+.015*math.sin(a)) for a in [i*math.tau/24 for i in range(25)]]
   line('Spectacle rim',points,.0011,'Steel','head')
   line('Spectacle temple',[(side*.064,-.251,z),(side*.078,-.14,z-.002)],.0011,'Steel','head')
  line('Spectacle bridge',[(-.006,-.251,1.748),(0,-.259,1.75),(.006,-.251,1.748)],.0011,'Steel','head')
  patch('Identity badge',[surface(-.13,1.36),surface(-.08,1.36),surface(-.08,1.29),surface(-.13,1.29)],'White')
 if role=='Courier':
  for side in (-1,1):
   line('Reflective chest band',[surface(side*x,1.32) for x in (.01,.07,.14,.18)],.009,'White')
   line('Backpack strap',[surface(side*.15,z) for z in (1.51,1.4,1.25,1.12)],.015,'Dark')
 tailor(role,body,arm,parts,mats,bvh,box,line,surface,patch,finish)
 # Join/normalize all four influences, preserve the same 21-bone bind skeleton.
 bpy.ops.object.select_all(action='DESELECT')
 for o in parts:o.select_set(True)
 bpy.context.view_layer.objects.active=body;bpy.ops.object.join();body=bpy.context.object;body.name='SK_City'+role
 for v in body.data.vertices:
  values=sorted([(g.group,g.weight) for g in v.groups if g.weight>.001],key=lambda v:-v[1])[:4]
  if not values:values=[(body.vertex_groups['pelvis'].index,1)]
  total=sum(w for g,w in values)
  for g in list(v.groups):body.vertex_groups[g.group].remove([v.index])
  for g,w in values:body.vertex_groups[g].add([v.index],w/total,'REPLACE')
 used=sorted(set(p.material_index for p in body.data.polygons));old_materials=list(body.data.materials);old_indices=[p.material_index for p in body.data.polygons]
 body.data.materials.clear()
 for index in used:body.data.materials.append(old_materials[index])
 remap={index:i for i,index in enumerate(used)}
 for p,index in zip(body.data.polygons,old_indices):p.material_index=remap[index]
 mod=body.modifiers.new('Role skeleton','ARMATURE');mod.object=arm;body.parent=arm
 bpy.ops.object.select_all(action='DESELECT');body.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
 bpy.ops.export_scene.fbx(filepath=str(out/(body.name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',use_mesh_modifiers=False)
 # Backpack exported independently so it can be hidden during car entry/seating.
 if role=='Courier':
  parts=[]
  pack=box('Courier backpack',(0,.215,1.29),(.36,.23,.44),'Red','spine_02',.035)
  for z in (1.14,1.44):box('Reflective backpack band',(0,.335,z),(.33,.008,.022),'White','spine_02',.002)
  box('Pack lid',(0,.22,1.523),(.36,.232,.045),'Dark','spine_02',.018)
  box('Insulated front pocket',(0,.343,1.29),(.26,.025,.19),'Red','spine_02',.02)
  for x in (-.15,.15):line('Pack stitched seam',[(x,.349,1.1),(x,.349,1.47)],.002,'Dark','spine_02')
  line('Backpack zip',[(-.13,.361,1.38),(.13,.361,1.38)],.002,'Steel','spine_02')
  line('Carry handle',[(-.06,.22,1.54),(-.05,.22,1.59),(.05,.22,1.59),(.06,.22,1.54)],.009,'Dark','spine_02')
  bpy.ops.object.select_all(action='DESELECT')
  for o in parts:o.select_set(True)
  bpy.context.view_layer.objects.active=pack;bpy.ops.object.join();pack=bpy.context.object;pack.name='SK_CityCourierPack'
  mod=pack.modifiers.new('Backpack rig','ARMATURE');mod.object=arm;pack.parent=arm
  bpy.ops.object.select_all(action='DESELECT');pack.select_set(True);arm.select_set(True)
  bpy.ops.export_scene.fbx(filepath=str(out/'SK_CityCourierPack.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,axis_forward='-Y',axis_up='Z',use_mesh_modifiers=False)
 bpy.ops.wm.save_as_mainfile(filepath='/tmp/CityRole_'+role+'.blend')
 records.append(dict(role=role,file=body.name+'.fbx',triangles=sum(len(p.vertices)-2 for p in body.data.polygons),bones=len(arm.data.bones),materials=[m.name for m in body.data.materials],palette=palette))
 print('ROLE_READY',records[-1]['role'],records[-1]['triangles'],flush=True)
if not args.role:
 hashes={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(out.iterdir()) if p.name.startswith(('SK_City','T_')) and p.suffix in ('.fbx','.png')}
 (out/'manifest.json').write_text(json.dumps(dict(version=3,face_revision=2,wardrobe_revision=3,roles=records,asset_hashes=hashes),indent=2))
