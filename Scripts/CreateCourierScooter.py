"""Original scooter, metres: body panels, chassis, tyres, discs, lights and mirrors."""
import bpy,math
from pathlib import Path
bpy.ops.wm.read_factory_settings(use_empty=True)
root=Path(__file__).resolve().parents[1]
colors={'paint':(.12,.18,.22,1),'rubber':(.025,.025,.03,1),'metal':(.35,.37,.4,1),'seat':(.055,.045,.04,1),'light':(.9,.92,.8,1),'red':(.7,.025,.015,1)}
mats={}
for name,color in colors.items():
 m=bpy.data.materials.new(name);m.diffuse_color=color;m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=color;p.inputs['Roughness'].default_value=.28 if name=='paint' else .7
 if name=='metal':p.inputs['Metallic'].default_value=.8
 mats[name]=m
parts=[]
def box(name,pos,size,mat,bevel=.03):
 bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.name=name;o.dimensions=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 o.data.materials.append(mats[mat]);mod=o.modifiers.new('Rounded manufactured edges','BEVEL');mod.width=bevel;mod.segments=3;bpy.ops.object.modifier_apply(modifier=mod.name);parts.append(o);return o
for x in (-.66,.65):
 bpy.ops.mesh.primitive_torus_add(major_radius=.19,minor_radius=.055,major_segments=32,minor_segments=10,location=(x,0,.245),rotation=(math.pi/2,0,0));o=bpy.context.object;o.data.materials.append(mats['rubber']);parts.append(o)
 bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=.14,depth=.09,location=(x,0,.245),rotation=(math.pi/2,0,0));o=bpy.context.object;o.data.materials.append(mats['metal']);parts.append(o)
box('Rear power unit',(-.52,0,.35),(.55,.35,.22),'metal')
box('Footboard',(.04,0,.2),(.85,.48,.08),'paint')
box('Rear body',(-.5,0,.47),(.92,.48,.4),'paint',.09)
box('Cushioned saddle',(-.22,0,.72),(.9,.45,.16),'seat',.065)
box('Front leg shield',(.45,0,.65),(.22,.5,.8),'paint',.08)
box('Front mudguard',(.65,0,.53),(.58,.18,.1),'paint')
box('Steering neck',(.48,0,1.04),(.12,.13,.32),'metal')
box('Handlebar',(.45,0,1.15),(.08,.69,.07),'metal')
for y in (-.31,.31):
 box('Rubber grip',(.45,y,1.15),(.095,.17,.085),'rubber')
 box('Mirror stalk',(.48,y,1.3),(.025,.025,.26),'metal',.008)
 box('Mirror glass',(.48,y,1.43),(.07,.15,.1),'metal',.02)
box('Headlamp',(.57,0,1.02),(.08,.28,.16),'light')
box('Rear lamp',(-.98,0,.66),(.05,.27,.11),'red')
box('Number plate',(-1,0,.5),(.025,.21,.13),'light',.005)
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name='SM_CityScooter'
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
bpy.ops.export_scene.fbx(filepath=str(root/'Assets/Vehicles/Courier/SM_CityScooter.fbx'),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False)
print('SCOOTER_READY',sum(len(p.vertices)-2 for p in o.data.polygons),tuple(o.dimensions))
