"""Original detailed courier scooter in metres. Blender -b -t 2 -P this_file.
Keep the previous wheelbase, seat and hand positions used by the gameplay rig.
"""
import bpy,math,json,hashlib
from pathlib import Path
from mathutils import Vector
root=Path(__file__).resolve().parents[1];out=root/'Assets/Vehicles/Courier';out.mkdir(parents=True,exist_ok=True)
bpy.ops.wm.read_factory_settings(use_empty=True)
palette={
 'Paint':dict(color=[.62,.013,.006,1],roughness=.24,metallic=.32),
 'Ivory':dict(color=[.78,.74,.64,1],roughness=.32,metallic=.12),
 'Trim':dict(color=[.012,.015,.02,1],roughness=.42,metallic=.12),
 'Rubber':dict(color=[.006,.007,.009,1],roughness=.88,metallic=0),
 'Seat':dict(color=[.018,.014,.013,1],roughness=.67,metallic=0),
 'Metal':dict(color=[.4,.44,.48,1],roughness=.26,metallic=.9),
 'Glass':dict(color=[.085,.15,.18,1],roughness=.13,metallic=.65),
 'LED':dict(color=[.88,.95,1,1],roughness=.18,metallic=.1,emission=.35),
 'Amber':dict(color=[1,.24,.006,1],roughness=.24,metallic=.1),
 'Tail':dict(color=[.45,.002,.004,1],roughness=.2,metallic=.2)}
mats={};parts=[]
for key,values in palette.items():
 m=bpy.data.materials.new('Scooter_'+key);m.diffuse_color=values['color'];m.use_nodes=True
 p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=values['color'];p.inputs['Roughness'].default_value=values['roughness'];p.inputs['Metallic'].default_value=values['metallic']
 if values.get('emission'):
  p.inputs['Emission Color'].default_value=values['color'];p.inputs['Emission Strength'].default_value=values['emission']
 mats[key]=m

def finish(o,name,mat,smooth=False):
 o.name=name;o.data.materials.append(mats[mat]);parts.append(o)
 if smooth:
  for p in o.data.polygons:p.use_smooth=True
 return o

def box(name,pos,size,mat,bevel=.015):
 bpy.ops.mesh.primitive_cube_add(size=1,location=pos);o=bpy.context.object;o.dimensions=size
 bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
 if bevel:
  mod=o.modifiers.new('Manufactured rounded edge','BEVEL');mod.width=bevel;mod.segments=2;bpy.ops.object.modifier_apply(modifier=mod.name)
  normal=o.modifiers.new('Panel normals','WEIGHTED_NORMAL');bpy.ops.object.modifier_apply(modifier=normal.name)
 return finish(o,name,mat)

def tube(name,points,r,mat,res=1):
 c=bpy.data.curves.new(name,'CURVE');c.dimensions='3D';c.bevel_depth=r;c.bevel_resolution=res
 sp=c.splines.new('POLY');sp.points.add(len(points)-1)
 for p,co in zip(sp.points,points):p.co=(*co,1)
 o=bpy.data.objects.new(name,c);bpy.context.collection.objects.link(o);bpy.context.view_layer.objects.active=o;o.select_set(True)
 bpy.ops.object.convert(target='MESH');return finish(bpy.context.object,name,mat,True)

def cylinder(name,a,b,r,mat,segments=32):
 a,b=Vector(a),Vector(b);d=b-a
 bpy.ops.mesh.primitive_cylinder_add(vertices=segments,radius=r,depth=d.length,location=(a+b)*.5)
 o=bpy.context.object;o.rotation_euler=d.to_track_quat('Z','Y').to_euler();return finish(o,name,mat,True)

def ring(name,pos,major,minor,mat,rotation=(math.pi/2,0,0),n=48):
 bpy.ops.mesh.primitive_torus_add(major_radius=major,minor_radius=minor,major_segments=n,minor_segments=6,location=pos,rotation=rotation)
 return finish(bpy.context.object,name,mat,True)

def shell(name,stations,mat,axis='X',segments=32):
 # Elliptical sections: longitudinal position, cross-section centre, two radii.
 v=[];faces=[]
 for p,c,r1,r2 in stations:
  for j in range(segments):
   a=j*math.tau/segments
   v.append((p,r1*math.cos(a),c+r2*math.sin(a)) if axis=='X' else (c+r1*math.sin(a),r2*math.cos(a),p))
 for i in range(len(stations)-1):
  for j in range(segments):
   a=i*segments+j;b=i*segments+(j+1)%segments;faces.append((a,b,b+segments,a+segments))
 faces.extend([tuple(reversed(range(segments))),tuple(range((len(stations)-1)*segments,len(v)))])
 mesh=bpy.data.meshes.new(name);mesh.from_pydata(v,[],faces);o=bpy.data.objects.new(name,mesh);bpy.context.collection.objects.link(o);return finish(o,name,mat,True)

# Tyres, cast five-spoke wheels, hubs, brake discs, calipers and shallow tread.
for x in (-.66,.65):
 ring('Road tyre',(x,0,.245),.188,.056,'Rubber',n=56)
 for y in (-.051,.051):
  ring('Rim lip',(x,y,.245),.143,.009,'Metal',n=40)
  ring('Tyre sidewall seam',(x,y*.85,.245),.195,.002,'Trim',n=40)
 for j in range(5):
  a=j*math.tau/5
  cylinder('Cast spoke',(x+math.cos(a)*.035,0,.245+math.sin(a)*.035),(x+math.cos(a+.15)*.14,0,.245+math.sin(a+.15)*.14),.012,'Metal',12)
 cylinder('Wheel hub',(x,-.065,.245),(x,.065,.245),.039,'Trim')
 cylinder('Ventilated disc',(x,-.078,.245),(x,-.068,.245),.104,'Metal',40)
 ring('Brake track',(x,-.08,.245),.086,.003,'Trim',n=32)
 for j in range(12):
  a=j*math.tau/12
  cylinder('Disc perforation',(x+math.cos(a)*.067,-.081,.245+math.sin(a)*.067),(x+math.cos(a)*.067,-.083,.245+math.sin(a)*.067),.0045,'Trim',8)
 box('Red brake caliper',(x+.075,-.091,.29),(.045,.033,.079),'Paint',.01)
 for j in range(36):
  a=j*math.tau/36
  for side in (-1,1):
   points=[]
   for k in range(4):
    t=k/3;angle=a+t*.08
    points.append((x+math.cos(angle)*(.244-.012*t),side*(.008+t*.035),.245+math.sin(angle)*(.244-.012*t)))
   tube('Tyre tread',points,.002,'Trim',1)
# Low central step-through footboard and visible underbody frame.
box('Underframe',(-.08,0,.19),(.89,.29,.075),'Trim',.025)
box('Red footboard',(.0,0,.245),(.89,.46,.065),'Paint',.035)
for side in (-1,1):
 box('Rubber foot mat',(.04,side*.135,.282),(.57,.145,.018),'Rubber',.015)
 for j in range(8):box('Mat drainage rib',(-.2+j*.066,side*.135,.293),(.009,.116,.005),'Trim',.002)
 tube('Footboard trim',[(-.43,side*.23,.27),(.28,side*.23,.27),(.42,side*.19,.31)],.008,'Ivory')
# Curved rear apron with split painted/ivory panels; retained saddle placement.
shell('Sculpted rear body',[(-.98,.5,.08,.09),(-.91,.52,.21,.17),(-.7,.54,.255,.19),(-.43,.54,.24,.18),(-.17,.53,.19,.17),(.02,.5,.11,.13)],'Paint')
box('Seat base',(-.3,0,.676),(.94,.432,.052),'Trim',.04)
shell('Two person stitched saddle',[(-.79,.72,.12,.045),(-.69,.745,.214,.069),(-.46,.751,.22,.071),(-.22,.747,.213,.062),(.01,.72,.185,.056),(.12,.701,.08,.025)],'Seat')
for side in (-1,1):
 tube('Seat piping',[(x,side*ry*2**-.5,z+rz*2**-.5) for x,z,ry,rz in [(-.79,.72,.12,.045),(-.69,.745,.214,.069),(-.46,.751,.22,.071),(-.22,.747,.213,.062),(.01,.72,.185,.056),(.12,.701,.08,.025)]],.0025,'Ivory')
 for x in (-.54,-.49,-.44,-.39,-.34,-.29):tube('Saddle stitched rib',[(x,-.19,.787),(x,-.1,.815),(x,0,.82),(x,.1,.815),(x,.19,.787)],.0013,'Trim',1)
 panel=box('Ivory side insert',(-.53,side*.267,.531),(.52,.015,.085),'Ivory',.028)
 for j in range(5):box('Engine vent',(-.75+j*.037,side*.26,.454),(.018,.008,.052),'Trim',.006)
 tube('Passenger grab rail',[(-.57,side*.258,.68),(-.81,side*.26,.68),(-.94,side*.19,.67),(-.97,0,.67)],.014,'Metal')
 cylinder('Rear shock',(-.67,side*.17,.29),(-.39,side*.17,.55),.017,'Metal',20)
 tube('Shock spring',[(-.67+t*.28+.023*math.cos(t*14*math.pi),side*.17+.023*math.sin(t*14*math.pi),.29+t*.26) for t in [i/120 for i in range(121)]],.005,'Trim',1)
# Detailed power unit, exhaust heat shield and centre stand.
box('Transmission housing',(-.58,.14,.315),(.53,.16,.13),'Trim',.045)
cylinder('Engine cover',(-.66,.19,.34),(-.66,.24,.34),.095,'Metal',32)
for j in range(4):box('Engine cooling rib',(-.48+j*.035,.24,.34),(.009,.018,.083),'Trim',.002)
cylinder('Exhaust',(-.86,-.22,.31),(-.33,-.22,.36),.048,'Trim')
cylinder('Exhaust tip',(-.9,-.22,.306),(-.86,-.22,.31),.035,'Metal')
box('Brushed heat shield',(-.56,-.261,.37),(.34,.018,.068),'Metal',.025)
for x in (-.68,-.44):cylinder('Heat shield screw',(x,-.275,.37),(x,-.279,.37),.008,'Trim',12)
tube('Folded centre stand',[(-.27,.09,.2),(-.48,.1,.13),(-.53,-.12,.13)],.012,'Trim')
# Full curved leg shield, steering column, sculpted mudguard and forks.
shell('Front curved shield',[(.3,.36,.12,.18),(.38,.38,.13,.245),(.55,.43,.085,.255),(.73,.48,.065,.235),(.93,.51,.075,.18),(1.01,.51,.067,.1)],'Paint',axis='Z')
for side in (-1,1):
 tube('Shield ivory piping',[(.34,side*.17,.32),(.40,side*.235,.43),(.46,side*.23,.68),(.51,side*.17,.91)],.009,'Ivory')
 cylinder('Fork lower',(.65,side*.09,.25),(.57,side*.09,.54),.022,'Metal')
 cylinder('Fork gaiter',(.57,side*.09,.53),(.51,side*.09,.77),.028,'Trim')
 for j in range(8):ring('Fork rubber rib',(.57-j*.0075,side*.09,.54+j*.026),.027,.003,'Rubber',rotation=(0,0,0),n=16)
shell('Front curved mudguard',[(.35,.46,.06,.02),(.44,.49,.09,.035),(.59,.51,.1,.042),(.73,.5,.095,.037),(.89,.455,.06,.022)],'Paint')
box('Inside glovebox',(.375,0,.65),(.035,.29,.18),'Trim',.03)
box('Glovebox latch',(.352,0,.735),(.012,.049,.015),'Metal',.005)
cylinder('Steering stem',(.48,0,.88),(.45,0,1.115),.035,'Trim')
shell('Handlebar front cowl',[(.37,1.125,.1,.06),(.45,1.14,.19,.087),(.54,1.135,.17,.08),(.61,1.13,.1,.045)],'Paint')
# Real headlamp housing, light lens and internal slats, turn signals.
box('Headlight surround',(.594,0,1.145),(.045,.235,.111),'Metal',.045)
box('Headlight lens',(.619,0,1.145),(.009,.203,.088),'LED',.035)
for z in (1.125,1.144,1.163):box('Headlamp lens rib',(.626,0,z),(.004,.175,.003),'Glass',.001)
for side in (-1,1):
 box('Front indicator',(.535,side*.188,.87),(.045,.045,.078),'Amber',.02)
 tube('Handlebar',[(.46,0,1.14),(.445,side*.24,1.15),(.43,side*.34,1.15)],.019,'Metal')
 cylinder('Grip',(.43,side*.265,1.15),(.43,side*.385,1.15),.029,'Rubber',24)
 for j in range(8):ring('Grip groove',(.43,side*(.273+j*.014),1.15),.028,.0015,'Trim',n=20)
 box('Handlebar switch',(.43,side*.248,1.154),(.071,.04,.06),'Trim',.01)
 box('Red switch',(.40,side*.248,1.174),(.018,.02,.012),'Paint',.003)
 tube('Brake lever',[(.48,side*.23,1.16),(.49,side*.31,1.175),(.47,side*.39,1.17)],.008,'Metal')
 tube('Mirror stalk',[(.47,side*.245,1.19),(.48,side*.285,1.33),(.46,side*.32,1.405)],.009,'Metal')
 box('Mirror housing',(.47,side*.34,1.416),(.047,.154,.098),'Trim',.04)
 box('Mirror reflective face',(.441,side*.34,1.416),(.006,.126,.074),'Glass',.027)
# Instrument cluster faces rider; native controls need no baked English text.
box('Instrument housing',(.367,0,1.16),(.02,.20,.095),'Trim',.026)
box('Digital display',(.352,0,1.16),(.008,.145,.062),'Glass',.014)
for j in range(3):box('Display indicator',(.346,-.04+j*.04,1.151),(.004,.015,.003),'LED',.001)
box('Tail lamp surround',(-.965,0,.58),(.032,.255,.115),'Trim',.025)
box('Tail lamp lens',(-.986,0,.59),(.015,.216,.075),'Tail',.025)
for side in (-1,1):box('Rear indicator',(-.974,side*.154,.585),(.025,.049,.052),'Amber',.015)
box('Rear licence bracket',(-.995,0,.442),(.024,.228,.15),'Trim',.008)
box('Rear plate',(-1.009,0,.442),(.004,.205,.123),'Ivory',.004)
# Join into the same single mesh path. Gameplay collision is deliberately separate.
bpy.ops.object.select_all(action='DESELECT')
for o in parts:o.select_set(True)
bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name='SM_CityScooter'
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
used=sorted(set(p.material_index for p in o.data.polygons));old=list(o.data.materials);indices=[p.material_index for p in o.data.polygons];o.data.materials.clear()
for i in used:o.data.materials.append(old[i])
remap={k:i for i,k in enumerate(used)}
for p,i in zip(o.data.polygons,indices):p.material_index=remap[i]
file=out/'SM_CityScooter.fbx'
bpy.ops.export_scene.fbx(filepath=str(file),use_selection=True,object_types={'MESH'},axis_forward='-Y',axis_up='Z',bake_anim=False)
manifest=dict(revision=2,triangles=sum(len(p.vertices)-2 for p in o.data.polygons),dimensions_m=list(o.dimensions),materials=palette,mesh_sha256=hashlib.sha256(file.read_bytes()).hexdigest(),author='Original CityLive project model',wheelbase_m=1.31,seat_height_m=.72)
(out/'manifest.json').write_text(json.dumps(manifest,indent=2))
bpy.ops.wm.save_as_mainfile(filepath='/tmp/CityCourierScooter.blend')
print('SCOOTER_READY',json.dumps(manifest),flush=True)
