"""Render actual generated role geometry together, without concept-art substitution."""
import bpy,math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1];roles=('Civilian','Police','Medic','Courier')
bpy.ops.wm.read_factory_settings(use_empty=True);rigs=[]
for i,role in enumerate(roles):
 with bpy.data.libraries.load('/tmp/CityRole_'+role+'.blend',link=False) as (source,dest):
  dest.objects=[n for n in source.objects if n.startswith(('SK_City','Armature'))]
 objects=[o for o in dest.objects if o]
 for o in objects:bpy.context.collection.objects.link(o)
 arm=next(o for o in objects if o.type=='ARMATURE');rigs.append(arm)
 for side,sign in [('l',1),('r',-1)]:
  for name,desired in [('upperarm_'+side,Vector((sign*.1,0,-1))),('lowerarm_'+side,Vector((sign*.05,-.05,-1)))]:
   pb=arm.pose.bones[name];m=pb.matrix.copy();rot=(pb.tail-pb.head).rotation_difference(desired.normalized());pivot=pb.head.copy();m=rot.to_matrix().to_4x4()@m;m.translation=pivot;pb.matrix=m;bpy.context.view_layer.update()
 arm.location.x=(i-1.5)*1.2
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.01));floor=bpy.context.object
m=bpy.data.materials.new('Studio');m.diffuse_color=(.035,.04,.045,1);floor.data.materials.append(m)
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('StudioWorld');scene.world.color=(.15,.15,.15)
for pos,power,size in [((2,-4,5),900,5),((-3,-2,3),700,4),((0,3,4),1100,5)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,1))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(0,-10,2.3));cam=bpy.context.object;cam.rotation_euler=(Vector((0,0,.95))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=5;scene.camera=cam
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=False;scene.render.resolution_percentage=100
scene.render.resolution_x=1600;scene.render.resolution_y=800;scene.render.filepath=str(root/'Assets/Characters/Roles/ActualModels.png');bpy.ops.render.render(write_still=True)
for i,arm in enumerate(rigs):arm.location.x=(i-1.5)*.42
cam.location=(0,-6,1.76);cam.rotation_euler=(Vector((0,-.13,1.76))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.ortho_scale=1.75
scene.render.resolution_x=1600;scene.render.resolution_y=480;scene.cycles.samples=80;scene.render.filepath=str(root/'Assets/Characters/Roles/Faces_Actual.png');bpy.ops.render.render(write_still=True)
print('ROLE_LINEUP_READY')
