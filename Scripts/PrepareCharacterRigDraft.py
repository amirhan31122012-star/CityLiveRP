"""Experimental asset-specific rig; visible defects remain. See Docs/CharacterSkinDraft.md."""
import bpy,sys,json,math
from mathutils import Vector
from pathlib import Path
args=sys.argv[sys.argv.index('--')+1:]
source_path=Path(args[0]).resolve()
output_path=Path(args[1]).resolve()
output_path.mkdir(parents=True,exist_ok=True)
if (output_path/'SK_CityBeige.fbx').exists(): raise RuntimeError('Choose an empty output directory.')
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.gltf(filepath=str(source_path))
o=next(o for o in bpy.context.scene.objects if o.type=='MESH');bpy.context.view_layer.objects.active=o;o.select_set(True)
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
lo=min(v.co.z for v in o.data.vertices)
for v in o.data.vertices:v.co.z-=lo
bpy.context.view_layer.objects.active=o
bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.mesh.remove_doubles(threshold=.00005);bpy.ops.object.mode_set(mode='OBJECT')
# Bake glTF texture transforms into UVs before exporting to FBX.
uv=o.data.uv_layers.active
for item in uv.data:
 item.uv.x=item.uv.x*15.952579498291016+.0000470000013592653
 item.uv.y=item.uv.y*16.002254486083984+.00002002716064453125
skin_vertices=set()
image=bpy.data.images.get('Image_1');pixels=list(image.pixels);width,height=image.size
for loop in o.data.loops:
    v=o.data.vertices[loop.vertex_index]
    if -.32<v.co.x<-.105 and .88<v.co.z<1.045:
        uv=o.data.uv_layers.active.data[loop.index].uv
        x=int((uv.x%1)*width)%width;y=int((uv.y%1)*height)%height
        r,g,b=pixels[(y*width+x)*4:(y*width+x)*4+3]
        if r>.075 and r>g*1.1 and b<g*.98:skin_vertices.add(v.index)
print('HAND_SKIN_VERTICES',len(skin_vertices))
bones={
'root':((0,0,0),(0,0,.1),None),
'pelvis':((0,0,.99),(0,0,1.1),'root'),
'spine_01':((0,0,1.1),(0,0,1.28),'pelvis'),
'spine_02':((0,0,1.28),(0,0,1.43),'spine_01'),
'spine_03':((0,0,1.43),(0,0,1.57),'spine_02'),
'neck_01':((0,0,1.57),(0,0,1.68),'spine_03'),
'head':((0,0,1.68),(0,0,1.88),'neck_01')}
for side,sgn in [('l',1),('r',-1)]:
 shoulder=(sgn*.18,0,1.54);elbow=(sgn*.26,-.06,1.27);hand=(.12,-.22,1.49) if side=='l' else (-.24,-.05,.99)
 bones['clavicle_'+side]=((sgn*.035,0,1.54),shoulder,'spine_03')
 bones['upperarm_'+side]=(shoulder,elbow,'clavicle_'+side)
 bones['lowerarm_'+side]=(elbow,hand,'upperarm_'+side)
 tip=Vector(hand)+(Vector(hand)-Vector(elbow)).normalized()*.095
 bones['hand_'+side]=(hand,tuple(tip),'lowerarm_'+side)
 hip=(sgn*.085,0,.99);knee=(sgn*.09,.07 if side=='l' else -.12,.55);ankle=(sgn*.105,.17 if side=='l' else -.17,.1)
 bones['thigh_'+side]=(hip,knee,'pelvis');bones['calf_'+side]=(knee,ankle,'thigh_'+side)
 bones['foot_'+side]=(ankle,(ankle[0],ankle[1]-.14,.045),'calf_'+side)
 bpy.ops.object.select_all(action='DESELECT')
a=bpy.data.armatures.new('CityBeigeSkeleton');arm=bpy.data.objects.new('Armature',a);bpy.context.collection.objects.link(arm);bpy.context.view_layer.objects.active=arm;arm.select_set(True)
bpy.ops.object.mode_set(mode='EDIT')
for name,(head,tail,parent) in bones.items():
 b=a.edit_bones.new(name);b.head=head;b.tail=tail
 if parent:b.parent=a.edit_bones[parent]
bpy.ops.object.mode_set(mode='OBJECT')
a.bones['root'].use_deform=False
segments=[(n,Vector(h),Vector(t)) for n,(h,t,p) in bones.items() if n!='root']
for n in bones:o.vertex_groups.new(name=n)
def dist(p,a,b):
 d=b-a;t=max(0,min(1,(p-a).dot(d)/d.length_squared));return (p-a-d*t).length
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.object.parent_set(type='ARMATURE_AUTO')
for m in list(o.modifiers):
 if m.type=='ARMATURE':o.modifiers.remove(m)
# Remove distant heat influences (hands must not drag trouser vertices).
for v in o.data.vertices:
    values=[]
    for g in list(v.groups):
        name=o.vertex_groups[g.group].name
        allowed=True
        if v.co.z>1.63: allowed=name in ('head','neck_01')
        elif v.co.z<.87: allowed=name.startswith(('thigh','calf','foot','pelvis'))
        elif name.startswith(('upperarm','lowerarm','hand','clavicle')):
            h,t,_=bones[name]
            allowed=dist(v.co,Vector(h),Vector(t))<.12
            if abs(v.co.x)<.14 and v.co.y>-.16:allowed=False
            if v.co.z<1.10 and v.co.x>-.21:allowed=False
        if allowed and g.weight>.003:values.append((g.group,g.weight))
    if v.index in skin_vertices:values=[(o.vertex_groups['hand_r'].index,1.)]
    values=sorted(values,key=lambda x:-x[1])[:4]
    if not values:
        nearest=min(segments,key=lambda x:dist(v.co,x[1],x[2]))[0]
        values=[(o.vertex_groups[nearest].index,1.)]
    total=sum(w for i,w in values)
    for g in list(v.groups):o.vertex_groups[g.group].remove([v.index])
    for i,w in values:o.vertex_groups[i].add([v.index],w/total,'REPLACE')
mod=o.modifiers.new('BodySkin','ARMATURE');mod.object=arm;o.parent=arm
# Unbend the photographed limbs into a neutral A pose, preserving skin weights.
bpy.context.view_layer.update()
for side,sgn in [('l',1),('r',-1)]:
 for name,target in [('upperarm_'+side,Vector((sgn*.55,0,-.83))),('lowerarm_'+side,Vector((sgn*.55,0,-.83))),('hand_'+side,Vector((sgn*.55,0,-.83)))]:
  pb=arm.pose.bones[name];current=(pb.tail-pb.head).normalized();q=current.rotation_difference(target.normalized());m=pb.matrix.copy();m=q.to_matrix().to_4x4()@m;m.translation=pb.head;pb.matrix=m;bpy.context.view_layer.update()
# Apply the armature deformation then make the neutral pose the bind pose.
edge_lengths={tuple(sorted(e.vertices)):(o.data.vertices[e.vertices[0]].co-o.data.vertices[e.vertices[1]].co).length for e in o.data.edges}
bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
for name,color in [('CityKnitRepair',(.34,.25,.17,1)),('CityPantsRepair',(.015,.012,.01,1))]:
    mat=bpy.data.materials.new(name);mat.diffuse_color=color;mat.use_nodes=True;mat.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=color;o.data.materials.append(mat)
# Split the scan's fused hand/trouser seam exposed by neutralizing the pose.
import bmesh
bm=bmesh.new();bm.from_mesh(o.data);bm.verts.ensure_lookup_table()
bad=[]
for f in bm.faces:
    for e in f.edges:
        old=edge_lengths.get(tuple(sorted(v.index for v in e.verts)),1.)
        if e.calc_length()>.06 and e.calc_length()>max(old*4.,.02):
            bad.append(f);break
print('CLEANUP_STRETCHED_FACES',len(bad))
bmesh.ops.delete(bm,geom=bad,context='FACES')
boundary=[e for e in bm.edges if e.is_boundary]
layer=bm.loops.layers.uv.active
new=bmesh.ops.holes_fill(bm,edges=boundary,sides=0).get('faces',[])
for f in new:
    f.material_index=1 if f.calc_center_median().z>1.1 else 2
    for l in f.loops:
        neighbors=[x[layer].uv.copy() for x in l.vert.link_loops if x.face not in new]
        if neighbors:l[layer].uv=sum(neighbors,Vector((0,0)))/len(neighbors)
bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces));bm.to_mesh(o.data);bm.free()

bpy.context.view_layer.objects.active=arm;bpy.ops.object.mode_set(mode='POSE');bpy.ops.pose.armature_apply(selected=False);bpy.ops.object.mode_set(mode='OBJECT')
mod=o.modifiers.new('BodySkin','ARMATURE');mod.object=arm
out=str(output_path)
for mat in o.data.materials:
 for node in mat.node_tree.nodes:
  if node.type=='TEX_IMAGE':
   for link in list(node.inputs['Vector'].links):mat.node_tree.links.remove(link)
# Save embedded images independently; UE material will be built explicitly.
for im in bpy.data.images:
 if im.type=='IMAGE' and im.size[0]>0:
  im.filepath_raw=out+'/'+im.name+'.png';im.file_format='PNG';im.save()
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.wm.save_as_mainfile(filepath=out+'/CityBeigeRig.blend')
bpy.ops.export_scene.fbx(filepath=out+'/SK_CityBeige.fbx',use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
assert all(abs(sum(g.weight for g in v.groups)-1)<.0001 for v in o.data.vertices)
print('RIG_OK',len(o.data.vertices),len(a.bones))
