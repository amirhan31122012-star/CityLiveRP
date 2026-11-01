"""Render the exported scooter mesh, using the same explicit palette as UE."""
import bpy,math,json
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1];out=root/'Assets/Vehicles/Courier'
bpy.ops.wm.read_factory_settings(use_empty=True);bpy.ops.import_scene.fbx(filepath=str(out/'SM_CityScooter.fbx'))
for m in bpy.data.materials:
 if m.name.startswith('Scooter_'):
  values=json.loads((out/'manifest.json').read_text())['materials'][m.name.removeprefix('Scooter_')]
  m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=values['color'];p.inputs['Roughness'].default_value=values['roughness'];p.inputs['Metallic'].default_value=values['metallic']
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.002));floor=bpy.context.object
m=bpy.data.materials.new('Studio');m.diffuse_color=(.025,.029,.035,1);floor.data.materials.append(m)
scene=bpy.context.scene;scene.world=bpy.data.worlds.new('StudioWorld');scene.world.color=(.17,.17,.17)
for pos,power,size in [((3,-4,5),900,4),((-3,-1,3),600,3),((0,3,4),900,3)]:
 bpy.ops.object.light_add(type='AREA',location=pos);l=bpy.context.object;l.data.energy=power;l.data.shape='DISK';l.data.size=size;l.rotation_euler=(Vector((0,0,.6))-l.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=2.65
scene.render.engine='CYCLES';scene.cycles.samples=48;scene.cycles.use_denoising=False
scene.render.resolution_x=1400;scene.render.resolution_y=1000;scene.render.resolution_percentage=100
for name,pos in [('ActualModel',(3.1,-4,2.1)),('ActualSide',(0,-5,1.2))]:
 cam.location=pos;cam.rotation_euler=(Vector((-.05,0,.69))-cam.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
print('SCOOTER_PREVIEW_READY')
