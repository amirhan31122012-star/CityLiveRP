"""Reconstruct separate trouser legs and align shoes on the prepared character."""
from pathlib import Path
import bpy,bmesh,json
from mathutils import Vector

def separate_legs(input_path, output_folder):
    output_folder=Path(output_folder);output_folder.mkdir(parents=True,exist_ok=True)
    bpy.ops.wm.open_mainfile(filepath=str(input_path))
    o=next(o for o in bpy.data.objects if o.type=='MESH');arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
    if arm.get('city_leg_version')==3:raise RuntimeError('Leg separation already applied.')
    bpy.context.view_layer.objects.active=o
    bm=bmesh.new();bm.from_mesh(o.data)
    bmesh.ops.bisect_plane(bm,geom=list(bm.verts)+list(bm.edges)+list(bm.faces),dist=.000001,plane_co=(0,0,0),plane_no=(1,0,0),clear_inner=True,clear_outer=False)
    uv_layer=bm.loops.layers.uv.active
    image=bpy.data.images['Image_1'];pixels=list(image.pixels);width,height=image.size
    best=(10,Vector((0,0)))
    for f in bm.faces:
     for loop in f.loops:
      if .35<loop.vert.co.z<.75:
       uv=loop[uv_layer].uv;x=int((uv.x%1)*width)%width;y=int((uv.y%1)*height)%height
       color=sum(pixels[(y*width+x)*4:(y*width+x)*4+3])
       if color<best[0]:best=(color,uv.copy())
    cap=bmesh.ops.holes_fill(bm,edges=[e for e in bm.edges if e.is_boundary],sides=0).get('faces',[])
    # Preserve the cap only below the crotch; the torso closes at the mirror seam.
    if cap:
     geo=set(cap)
     for f in cap:geo.update(f.edges);geo.update(f.verts)
     bmesh.ops.bisect_plane(bm,geom=list(geo),dist=.000001,plane_co=(0,0,.90),plane_no=(0,0,1),clear_inner=False,clear_outer=False)
     # New split faces keep the original cap's unique material tag until cleanup.
     # Identify cap surfaces by all vertices lying on the sagittal plane.
     cap=[f for f in bm.faces if all(abs(v.co.x)<.00001 for v in f.verts)]
     remove=[f for f in cap if f.calc_center_median().z>.90001]
     bmesh.ops.delete(bm,geom=remove,context='FACES_ONLY')
     for f in cap:
      if not f.is_valid:continue
      for loop in f.loops:loop[uv_layer].uv=best[1]
      f.material_index=0
    for v in bm.verts:
     t=max(0,min(1,(.96-v.co.z)/.16));t=t*t*(3-2*t)
     v.co.x+=.04*t
    bmesh.ops.triangulate(bm,faces=[f for f in bm.faces if len(f.verts)>4]);bmesh.ops.recalc_face_normals(bm,faces=list(bm.faces))
    bm.to_mesh(o.data);bm.free()
    # Align the scanned shoe to the foot bone, instead of mirroring an inward toe.
    import numpy as np,math
    samples=np.array([[v.co.x,v.co.y] for v in o.data.vertices if v.co.z<.13])
    values,axes=np.linalg.eigh(np.cov(samples.T));axis=axes[:,np.argmax(values)]
    if axis[1]>0:axis=-axis
    angle=-math.pi/2-math.atan2(axis[1],axis[0])
    ankle=[v.co for v in o.data.vertices if .16<v.co.z<.22]
    knee=[v.co for v in o.data.vertices if .53<v.co.z<.57]
    pivot=sum(ankle,Vector())/len(ankle);pivot.z=.10
    knee_center=sum(knee,Vector())/len(knee);knee_center.z=.55
    cosine,sine=math.cos(angle),math.sin(angle)
    for v in o.data.vertices:
        t=max(0,min(1,(.24-v.co.z)/.10));t=t*t*(3-2*t)
        if t:
            dx,dy=v.co.x-pivot.x,v.co.y-pivot.y
            target=Vector((pivot.x+cosine*dx-sine*dy,pivot.y+sine*dx+cosine*dy,v.co.z))
            v.co=v.co.lerp(target,t)
    foot_x=[v.co.x for v in o.data.vertices if v.co.z<.13]
    width_scale=min(1.,.12/(max(foot_x)-min(foot_x)))
    for v in o.data.vertices:
        t=max(0,min(1,(.24-v.co.z)/.10));t=t*t*(3-2*t)
        v.co.x=pivot.x+(v.co.x-pivot.x)*(1-t+t*width_scale)
    correction=.13-pivot.x
    for v in o.data.vertices:
        t=max(0,min(1,(.30-v.co.z)/.15));t=t*t*(3-2*t);v.co.x+=correction*t
    pivot.x=.13
    print('SHOE_ALIGNMENT_DEGREES',math.degrees(angle),'WIDTH_SCALE',width_scale)
    
    mirror=o.modifiers.new('SeparatedLegs','MIRROR');mirror.merge_threshold=.0001;mirror.use_clip=True;mirror.use_mirror_vertex_groups=True
    bpy.ops.object.modifier_move_to_index(modifier=mirror.name,index=0);bpy.ops.object.modifier_apply(modifier=mirror.name)
    bpy.context.view_layer.objects.active=arm;bpy.ops.object.mode_set(mode='EDIT')
    arm.data.edit_bones['thigh_l'].tail=knee_center
    arm.data.edit_bones['calf_l'].head=knee_center
    arm.data.edit_bones['calf_l'].tail=pivot
    arm.data.edit_bones['foot_l'].head=pivot
    arm.data.edit_bones['foot_l'].tail=pivot+Vector((0,-.14,-.055))
    for b in list(arm.data.edit_bones):
     if b.name.endswith('_l'):
      other=arm.data.edit_bones.get(b.name[:-2]+'_r')
      if other:other.head=Vector((-b.head.x,b.head.y,b.head.z));other.tail=Vector((-b.tail.x,b.tail.y,b.tail.z));other.roll=-b.roll
    bpy.ops.object.mode_set(mode='OBJECT')
    # Reweight each trouser leg independently. No influence from the opposite leg.
    for mod in list(o.modifiers):
     if mod.type=='ARMATURE':o.modifiers.remove(mod)
    o.vertex_groups.clear();segments=[]
    for bone in arm.data.bones:
     if bone.name=='root':continue
     radius=.10 if bone.name.startswith('spine') else .12 if bone.name=='pelvis' else .09 if bone.name=='head' else .07 if bone.name.startswith('thigh') else .045
     group=o.vertex_groups.new(name=bone.name);segments.append((group.index,bone.name,bone.head_local.copy(),bone.tail_local.copy(),radius))
    def distance(p,a,b):
     d=b-a;t=max(0,min(1,(p-a).dot(d)/d.length_squared));return (p-a-d*t).length
    for v in o.data.vertices:
     scores=[]
     for index,name,a,b,radius in segments:
      if v.co.z>1.68 and name not in ('head','neck_01'):continue
      if v.co.z<.8 and not name.startswith(('thigh','calf','foot','pelvis')):continue
      if name.endswith('_l') and v.co.x<-.025:continue
      if v.co.z<.94 and name.startswith(('thigh','calf','foot')) and ((v.co.x>0 and name.endswith('_r')) or (v.co.x<0 and name.endswith('_l'))):continue
      if name.endswith('_r') and v.co.x>.025:continue
      scores.append((1/max(distance(v.co,a,b)-radius*.5,.015)**4,index))
     scores=sorted(scores,reverse=True)[:4];total=sum(w for w,i in scores)
     for w,i in scores:o.vertex_groups[i].add([v.index],w/total,'REPLACE')
    mod=o.modifiers.new('BodySkin','ARMATURE');mod.object=arm
    assert all(abs(sum(g.weight for g in v.groups)-1)<.001 for v in o.data.vertices)
    # No shared trouser vertices/edges below the crotch and no cross-leg weights.
    assert not any(abs(v.co.x)<.02 and v.co.z<.84 for v in o.data.vertices)
    for v in o.data.vertices:
     if v.co.z<.8:
      assert all(not(o.vertex_groups[g.group].name.endswith('_r') and v.co.x>0) and not(o.vertex_groups[g.group].name.endswith('_l') and v.co.x<0) for g in v.groups)
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True);arm.select_set(True);bpy.context.view_layer.objects.active=arm
    arm['city_leg_version']=3
    bpy.ops.wm.save_as_mainfile(filepath=str(output_folder/'CityBeigeRig.blend'))
    bpy.ops.export_scene.fbx(filepath=str(output_folder/'SK_CityBeige.fbx'),use_selection=True,object_types={'MESH','ARMATURE'},add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=False,axis_forward='-Y',axis_up='Z',apply_unit_scale=True)
    print('LEG_FIX_OK',len(o.data.vertices),len(o.data.polygons),best[0])

if __name__ == '__main__':
    import sys
    args=sys.argv[sys.argv.index('--')+1:]
    separate_legs(args[0],args[1])
