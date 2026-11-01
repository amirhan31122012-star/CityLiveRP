"""Render generated meshes: blender -b -P Scripts/PreviewRoleCharacters.py -- Civilian."""
import bpy,sys,math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1]
role=sys.argv[-1]
if role not in ('Civilian','Police','Medic','Courier'):raise ValueError('Pass -- Civilian, Police, Medic or Courier')
bpy.ops.wm.open_mainfile(filepath='/tmp/CityRole_'+role+'.blend')
arm=next(o for o in bpy.data.objects if o.type=='ARMATURE')
# Render the actual exported mesh in a relaxed pose, keeping the bind rig untouched.
for side,sign in [('l',1),('r',-1)]:
 for name,desired in [('upperarm_'+side,Vector((sign*.1,0,-1))),('lowerarm_'+side,Vector((sign*.05,-.05,-1)))]:
  pb=arm.pose.bones[name];m=pb.matrix.copy();rot=(pb.tail-pb.head).rotation_difference(desired.normalized());pivot=pb.head.copy();m=rot.to_matrix().to_4x4()@m;m.translation=pivot;pb.matrix=m;bpy.context.view_layer.update()
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.01));floor=bpy.context.object
m=bpy.data.materials.new('Studio');m.diffuse_color=(.035,.04,.045,1);floor.data.materials.append(m)
scene=bpy.context.scene;scene.world=bpy.data.worlds.new("StudioWorld");scene.world.color=(.2,.2,.2)
for pos,power,size in [((2,-4,4),600,3),((-3,-2,2.5),350,3),((0,3,3),650,2)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,1))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(2,-6,2.5));cam=bpy.context.object;cam.rotation_euler=(Vector((0,-.02,1))-cam.location).to_track_quat('-Z','Y').to_euler();cam.data.type='ORTHO';cam.data.ortho_scale=2.25;scene.camera=cam
scene.render.engine='CYCLES';scene.cycles.samples=32;scene.cycles.use_denoising=False;scene.render.resolution_x=400;scene.render.resolution_y=640;scene.render.resolution_percentage=100
scene.render.filepath=str(root/'Assets/Characters/Roles'/(role+'_Actual.png'));bpy.ops.render.render(write_still=True)
