"""Asset-specific reconstruction and rig for Casual Beige Knit Look (CC BY 4.0).
Run in Blender: --python PrepareCharacterRig.py -- original.glb empty_output_folder
See Assets/Characters/Beige/README.md for attribution and modifications.
"""
import bpy,sys,json,math
from mathutils import Vector
from pathlib import Path
args=sys.argv[sys.argv.index('--')+1:]
source_path=Path(args[0]).resolve();output_path=Path(args[1]).resolve()
output_path.mkdir(parents=True,exist_ok=True)
if (output_path/'SK_CityBeige.fbx').exists():raise RuntimeError('Choose a new output folder.')
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
colors={}
im=bpy.data.images['Image_1'];pix=list(im.pixels);w,h=im.size
for loop in o.data.loops:
    uv=o.data.uv_layers.active.data[loop.index].uv
    x=int((uv.x%1)*w)%w;y=int((uv.y%1)*h)%h
    colors[loop.vertex_index]=pix[(y*w+x)*4:(y*w+x)*4+3]
bones={
'root':((0,0,0),(0,0,.1),None),
'pelvis':((0,0,.99),(0,0,1.1),'root'),
'spine_01':((0,0,1.1),(0,0,1.28),'pelvis'),
'spine_02':((0,0,1.28),(0,0,1.43),'spine_01'),
'spine_03':((0,0,1.43),(0,0,1.57),'spine_02'),
'neck_01':((0,0,1.57),(0,0,1.68),'spine_03'),
'head':((0,0,1.68),(0,0,1.88),'neck_01')}
for side,sgn in [('l',1),('r',-1)]:
 shoulder=(sgn*.18,0,1.54);elbow=(.24,-.10,1.24) if side=='l' else (-.265,-.055,1.23);hand=(.08,-.24,1.445) if side=='l' else (-.255,-.10,.955)
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
for v in o.data.vertices:
    values=[]
    for g in list(v.groups):
        name=o.vertex_groups[g.group].name;allowed=True
        if v.co.z>1.63:allowed=name in ('head','neck_01')
        elif v.co.z<.8:allowed=name.startswith(('thigh','calf','foot','pelvis'))
        elif name.startswith(('upperarm','lowerarm','hand','clavicle')):
            h,t,_=bones[name];allowed=dist(v.co,Vector(h),Vector(t))<.105
            if name.endswith('_r') and v.co.z<1.16 and v.co.x>-.195:allowed=False
            if name.endswith('_r') and v.co.z<1.02 and max(colors.get(v.index,[0]))<.10:allowed=False
            if abs(v.co.x)<.18 and v.co.y>-.17:allowed=False
            if name in ('lowerarm_l','hand_l') and v.co.x<.22 and v.co.y>-.16:allowed=False
        if allowed and g.weight>.003:values.append((g.group,g.weight))
    if .83<v.co.z<.96 and v.co.x<-.195 and max(colors.get(v.index,[0]))>.13:values=[(o.vertex_groups['hand_r'].index,1.)]
    values=sorted(values,key=lambda x:-x[1])[:4]
    if not values:
        candidates=[x for x in segments if not x[0].startswith(('upperarm','lowerarm','hand','clavicle'))]
        values=[(o.vertex_groups[min(candidates,key=lambda x:dist(v.co,x[1],x[2]))[0]].index,1.)]
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
bpy.context.view_layer.objects.active=o;bpy.ops.object.modifier_apply(modifier=mod.name)
group=o.vertex_groups.new(name='SurfaceRepair')
for v in o.data.vertices:
    if v.co.x>.10 and 1.20<v.co.z<1.58:
        strength=max(0,1-abs(v.co.z-1.39)/.19)
        group.add([v.index],strength,'REPLACE')
smooth=o.modifiers.new('RepairSleeve','SMOOTH');smooth.factor=1;smooth.iterations=60;smooth.vertex_group=group.name
bpy.ops.object.modifier_apply(modifier=smooth.name)
o.vertex_groups.remove(o.vertex_groups['SurfaceRepair'])

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
# Reconstruct the damaged side from the clean side and mirror matching bones.
import bmesh
bpy.context.view_layer.objects.active=o
for v in o.data.vertices:
    t=max(0,min(1,(v.co.z-1.55)/.13));t=t*t*(3-2*t);v.co.x-=.045*t
bm=bmesh.new();bm.from_mesh(o.data)
bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.000001,plane_co=(0,0,0),plane_no=(1,0,0),clear_inner=True,clear_outer=False)
bm.to_mesh(o.data);bm.free()
mirror=o.modifiers.new('ReconstructedRightSide','MIRROR');mirror.use_clip=True;mirror.use_mirror_vertex_groups=True;mirror.merge_threshold=.0001
bpy.ops.object.modifier_move_to_index(modifier=mirror.name,index=0);bpy.ops.object.modifier_apply(modifier=mirror.name)
bpy.context.view_layer.objects.active=arm;bpy.ops.object.mode_set(mode='EDIT')
for b in list(arm.data.edit_bones):
 if b.name.endswith('_l'):
  other=arm.data.edit_bones.get(b.name[:-2]+'_r')
  if other:
   other.head=Vector((-b.head.x,b.head.y,b.head.z));other.tail=Vector((-b.tail.x,b.tail.y,b.tail.z));other.roll=-b.roll
bpy.ops.object.mode_set(mode='OBJECT')

# Recalculate weights in the reconstructed neutral pose, without folded-arm influences.
for mod in list(o.modifiers):
 if mod.type=='ARMATURE':o.modifiers.remove(mod)
o.vertex_groups.clear()
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
from mathutils import Vector
segments=[]
for bone in arm.data.bones:
    if bone.name=='root':continue
    radius=.10 if bone.name.startswith('spine') else .12 if bone.name=='pelvis' else .09 if bone.name=='head' else .07 if bone.name.startswith('thigh') else .045
    group=o.vertex_groups.new(name=bone.name)
    segments.append((group.index,bone.name,bone.head_local.copy(),bone.tail_local.copy(),radius))
def distance(p,a,b):
    d=b-a;t=max(0,min(1,(p-a).dot(d)/d.length_squared));return (p-a-d*t).length
for v in o.data.vertices:
    scores=[]
    for index,name,a,b,radius in segments:
        if v.co.z>1.68 and name not in ('head','neck_01'):continue
        if v.co.z<.8 and not name.startswith(('thigh','calf','foot','pelvis')):continue
        if name.endswith('_l') and v.co.x<-.025:continue
        if name.endswith('_r') and v.co.x>.025:continue
        scores.append((1/max(distance(v.co,a,b)-radius*.5,.015)**4,index))
    scores=sorted(scores,reverse=True)[:4];total=sum(w for w,i in scores)
    for w,i in scores:o.vertex_groups[i].add([v.index],w/total,'REPLACE')
mod=o.modifiers.new('BodySkin','ARMATURE');mod.object=arm

# Mirror merges can combine both sets of seam influences. Limit and normalize.
for vertex in o.data.vertices:
    weights=sorted([(g.group,g.weight) for g in vertex.groups],key=lambda item:-item[1])
    keep=weights[:4];total=sum(w for index,w in keep)
    for index,weight in weights:o.vertex_groups[index].remove([vertex.index])
    for index,weight in keep:o.vertex_groups[index].add([vertex.index],weight/total,'REPLACE')
assert all(p.material_index==0 for p in o.data.polygons)
while len(o.data.materials)>1:o.data.materials.pop(index=len(o.data.materials)-1)
bpy.ops.object.select_all(action='DESELECT');o.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
bpy.ops.wm.save_as_mainfile(filepath=out+'/CityBeigeRig.blend')
bpy.ops.export_scene.fbx(filepath=out+'/SK_CityBeige.fbx',use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
assert all(abs(sum(g.weight for g in v.groups)-1)<.0001 for v in o.data.vertices)
print('RIG_OK',len(o.data.vertices),len(arm.data.bones))
