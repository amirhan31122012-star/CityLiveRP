"""Small scene cards from our existing district geometry; not gameplay captures."""
import bpy,math
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1];out=root/'Assets/UI/Spawn';out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.open_mainfile(filepath=str(root/'Assets/Maps/PoliceDistrictRealistic/CityLive_PoliceDistrict_Realistic.blend'))
scene=bpy.context.scene
for o in list(scene.objects):
 if o.type=='CAMERA':bpy.data.objects.remove(o,do_unlink=True)
bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam;cam.data.lens=28
scene.render.engine='CYCLES';scene.cycles.samples=24;scene.cycles.use_denoising=False
scene.render.resolution_x=400;scene.render.resolution_y=640;scene.render.resolution_percentage=100
views=[('Organization',(-103,30,5),(-78,48,5)),('Start',(-135,0,3),(-78,48,5)),('Last',(-18,-12,3),(-74,-62,6)),('Parking',(-95,10,3),(-78,24,.3)),('Home',(28,37,3),(52,64,8))]
for name,pos,target in views:
 cam.location=pos;cam.rotation_euler=(Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
 scene.render.filepath=str(out/(name+'.png'));bpy.ops.render.render(write_still=True)
print('SPAWN_CARDS_READY')
