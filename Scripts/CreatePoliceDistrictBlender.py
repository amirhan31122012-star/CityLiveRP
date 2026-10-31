"""Original Russian district preview, metres; does not modify Unreal maps.

blender -b --python Scripts/CreatePoliceDistrictBlender.py -- OUTPUT_DIRECTORY
Creates editable .blend, portable .glb, geometry statistics and three previews.
"""
import bpy
import json
import math
from mathutils import Vector
from pathlib import Path
import random
import sys

sys.path.insert(0, str(Path(__file__).parent))
from CityDistrictBatch import export_batched_glb

OUT = Path(sys.argv[sys.argv.index('--') + 1]).resolve()
OUT.mkdir(parents=True, exist_ok=True)
random.seed(311)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)
scene = bpy.context.scene
scene.unit_settings.system = 'METRIC'
materials, cache = {}, {}


def material(name, rgb, metallic=0, roughness=.75):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*rgb, 1)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get('Principled BSDF')
    bsdf.inputs['Base Color'].default_value = (*rgb, 1)
    bsdf.inputs['Metallic'].default_value = metallic
    bsdf.inputs['Roughness'].default_value = roughness
    materials[name] = mat


for name, rgb in {
    'asphalt': (.09, .105, .12), 'concrete': (.47, .48, .45),
    'paving': (.56, .52, .45), 'white': (.85, .86, .79),
    'grass': (.19, .28, .105), 'soil': (.19, .145, .095),
    'cream': (.73, .68, .54), 'police_wall': (.63, .65, .61),
    'brick': (.42, .20, .12), 'brick_light': (.65, .39, .21),
    'blue': (.025, .12, .30), 'red': (.57, .035, .025),
    'roof': (.17, .19, .20), 'wood': (.27, .13, .055),
    'leaf1': (.15, .29, .075), 'leaf2': (.24, .36, .10),
    'leaf3': (.09, .22, .055), 'yellow': (.85, .60, .07),
    'panel_joint': (.40, .41, .37), 'playground': (.48, .21, .13),
}.items():
    material(name, rgb)
material('glass', (.105, .20, .26), .65, .22)
material('metal', (.16, .19, .20), .65, .4)


def box(name, loc, size, mat, rotation=0):
    key = (tuple(size), mat)
    if key not in cache:
        x, y, z = (v / 2 for v in size)
        verts = [(-x,-y,-z), (x,-y,-z), (x,y,-z), (-x,y,-z),
                 (-x,-y,z), (x,-y,z), (x,y,z), (-x,y,z)]
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(verts, [], [(0,3,2,1), (4,5,6,7), (0,1,5,4),
                                   (1,2,6,5), (2,3,7,6), (3,0,4,7)])
        mesh.materials.append(materials[mat])
        cache[key] = mesh
    obj = bpy.data.objects.new(name, cache[key])
    scene.collection.objects.link(obj)
    obj.location, obj.rotation_euler.z = loc, rotation
    return obj


def cylinder(name, loc, radius, depth, mat, segments=10):
    key = ('cylinder', radius, depth, mat, segments)
    if key not in cache:
        verts = [(radius*math.cos(i*math.tau/segments), radius*math.sin(i*math.tau/segments), z)
                 for z in (-depth/2, depth/2) for i in range(segments)]
        faces = [tuple(reversed(range(segments))), tuple(range(segments, segments*2))]
        faces += [(i, (i+1)%segments, (i+1)%segments+segments, i+segments) for i in range(segments)]
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata(verts, [], faces)
        mesh.materials.append(materials[mat])
        cache[key] = mesh
    obj = bpy.data.objects.new(name, cache[key])
    scene.collection.objects.link(obj)
    obj.location = loc
    return obj


font_path = '/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf'
font = bpy.data.fonts.load(font_path) if Path(font_path).is_file() else None


def label(text, loc, size, mat='white'):
    data = bpy.data.curves.new('Sign_' + text, 'FONT')
    data.body, data.size, data.align_x = text, size, 'CENTER'
    data.extrude = .005
    if font:
        data.font = font
    data.materials.append(materials[mat])
    obj = bpy.data.objects.new('Sign_' + text, data)
    scene.collection.objects.link(obj)
    obj.location = loc
    obj.rotation_euler = (math.pi/2, 0, 0)  # readable from south
    return obj


box('District_ground', (0,0,-.32), (420,340,.6), 'grass')
# Two continuous streets and a perimeter loop: crossings stay open.
for y, width in ((0,14), (108,10), (-108,10)):
    box('Road_EW', (0,y,-.055), (410,width,.1), 'asphalt')
    for side in (-1,1):
        yy = y + side*(width/2+2)
        box('Sidewalk_EW', (0,yy,.055), (284,4,.12), 'concrete')
        box('Curb_EW', (0,y+side*(width/2+.1),.12), (284,.2,.24), 'white')
    for x in range(-195,200,9):
        if abs(x) < 14 or abs(abs(x)-142) < 12:
            continue
        box('Lane_dash', (x,y,.008), (4,.13,.012), 'white')
for x, width in ((0,12), (-142,10), (142,10)):
    box('Road_NS', (x,0,-.054), (width,330,.1), 'asphalt')
    for side in (-1,1):
        for yy, length in ((54,88), (-54,88)):
            box('Sidewalk_NS', (x+side*(width/2+2),yy,.055), (4,length,.12), 'concrete')
            box('Curb_NS', (x+side*(width/2+.1),yy,.12), (.2,length,.24), 'white')
    for y in range(-150,154,9):
        if abs(y) < 14 or abs(abs(y)-108) < 12:
            continue
        box('Lane_dash', (x,y,.01), (.13,4,.012), 'white')
# Remove the east-west sidewalk across intersecting streets instead of creating obstacles.
for obj in list(scene.objects):
    if obj.name.startswith(('Sidewalk_EW', 'Curb_EW')):
        x, y, z = obj.location
        sy, sz = obj.dimensions.y, obj.dimensions.z
        mat = 'white' if obj.name.startswith('Curb') else 'concrete'
        bpy.data.objects.remove(obj, do_unlink=True)
        for a,b in ((-136,-6), (6,136)):
            box('Walk_segment', ((a+b)/2,y,z), (b-a,sy,sz), mat)
for y in (-16,16):
    for x in range(-5,6,2):
        box('Zebra', (x,y,.012), (1,4,.014), 'white')
for x in (-17,17):
    for y in range(-6,7,2):
        box('Zebra', (x,y,.014), (4,1,.014), 'white')
for x in (-25,25):
    box('Stop_line', (x,-3.5 if x < 0 else 3.5,.012), (.35,6,.015), 'white')


def windows(cx, cy, width, depth, floors, police=False, tone='cream'):
    height = floors * 3.1
    box('Police_building' if police else 'Apartment_building',
        (cx,cy,height/2+.2), (width,depth,height), 'police_wall' if police else tone)
    box('Foundation', (cx,cy,.45), (width+.12,depth+.12,.8), 'roof')
    box('Flat_roof', (cx,cy,height+.35), (width+.6,depth+.6,.3), 'roof')
    for side in (-1,1):
        yy = cy + side*(depth/2+.04)
        for floor in range(floors):
            z = 2+floor*3.1
            box('Floor_joint', (cx,yy,z-1.2), (width,.035,.045), 'panel_joint')
            for col in range(int(width/3.4)):
                xx = cx-width/2+2+col*3.4
                if police and side == -1 and abs(xx-cx) < 6 and floor == 0:
                    continue
                box('Window_frame', (xx,yy,z), (1.65,.10,1.65), 'white')
                box('Window_glass', (xx,yy+side*.065,z), (1.43,.045,1.43), 'glass')
                box('Window_mullion', (xx,yy+side*.10,z), (.055,.05,1.46), 'white')
                box('Window_sill', (xx,yy+side*.13,z-.83), (1.85,.34,.09), 'white')
                if not police and col%4==1:
                    box('Balcony_slab', (xx,yy+side*.65,z-.95), (2.6,1.25,.17), 'concrete')
                    box('Balcony_front', (xx,yy+side*1.22,z-.48), (2.6,.12,.8), tone)
                    for ss in (-1,1):
                        box('Balcony_side', (xx+ss*1.24,yy+side*.65,z-.48), (.12,1.2,.8), tone)
        for xx in range(int(cx-width/2+1), int(cx+width/2), 7):
            box('Facade_joint', (xx,yy,height/2), (.045,.03,height-.5), 'panel_joint')
    for side in (-1,1):
        xx = cx + side*(width/2+.035)
        for floor in range(floors):
            for dy in (-depth*.26, depth*.26):
                box('Side_window_frame', (xx,cy+dy,2+floor*3.1), (.10,1.6,1.6), 'white')
                box('Side_window', (xx+side*.06,cy+dy,2+floor*3.1), (.04,1.4,1.4), 'glass')
    for dx in (-width*.3, width*.3):
        box('Roof_vent', (cx+dx,cy,height+.95), (1.4,1.4,1.1), 'concrete')


# Police station: public entrance faces the avenue, service yard at the back.
windows(-78,62,58,28,3,police=True)
box('Police_blue_band', (-78,47.78,6.65), (58,.24,1.4), 'blue')
label('ПОЛИЦИЯ', (-78,47.61,6.22), 1.18)
box('Entrance_portal', (-78,46.8,2.65), (11,2.5,5.2), 'cream')
box('Entrance_glass', (-78,45.50,2.5), (6,.10,3.4), 'glass')
for dx in (-3,0,3):
    box('Door_frame', (-78+dx,45.40,2.5), (.12,.12,3.5), 'white')
box('Entrance_canopy', (-78,44.7,4.75), (12,5,.32), 'blue')
for dx in (-5,5):
    cylinder('Canopy_column', (-78+dx,43,2.25), .15,4.5,'metal')
for i in range(4):
    box('Entrance_step', (-78,43.5-i*.5,.09*(4-i)), (10,1.1,.18*(4-i)), 'concrete')
box('Access_path', (-78,38,.05), (12,10,.1), 'paving')
box('Police_parking', (-78,24,-.01), (94,18,.1), 'asphalt')
for x in range(-119,-32,4):
    box('Parking_line', (x,23,.055), (.11,6,.012), 'white')
label('ДЕЖУРНАЯ ЧАСТЬ  •  102', (-78,45.31,4.06), .30)
box('Police_plaque', (-85,47.70,2.2), (1.25,.12,1.7), 'blue')
label('МВД', (-85,47.61,2.35), .28, 'yellow')
label('РОССИИ', (-85,47.61,1.99), .18)
for x in (-100,-56):
    cylinder('Flagpole', (x,43,4.7), .055,9.4,'metal')
    for z, mat in ((8.5,'white'), (8.05,'blue'), (7.6,'red')):
        box('Russian_flag', (x+1,43,z), (2,.035,.45), mat)
# Three varied residential courts, enough drivable space between buildings.
windows(77,73,80,17,5,tone='cream')
windows(116,47,17,35,5,tone='brick_light')
windows(-79,-68,80,17,5,tone='brick')
windows(-120,-44,17,31,5,tone='cream')
windows(54,-67,27,23,9,tone='cream')
windows(104,-72,28,23,9,tone='police_wall')
for cx,cy,w in ((77,73,80), (-79,-68,80), (54,-67,27), (104,-72,28)):
    for dx in (-w*.26,w*.26):
        yy = cy-12 if cy < 0 and w < 30 else cy-9
        box('Residential_entry', (cx+dx,yy,1.3), (2.5,.25,2.4), 'blue')
        box('Entry_canopy', (cx+dx,yy-.6,2.7), (3,1.8,.18), 'roof')
# Small local shops, with separate doors/windows and readable shop fronts.
for x,y,sign in ((48,29,'ПРОДУКТЫ'), (-107,-25,'АПТЕКА'), (105,-28,'ПЕКАРНЯ')):
    box('Local_shop', (x,y,2), (24,12,4), 'cream')
    box('Shop_roof', (x,y,4.12), (24.5,12.5,.24), 'roof')
    box('Shop_glazing', (x,y-6.08,1.8), (21,.1,2.6), 'glass')
    for dx in (-10,-5,0,5,10):
        box('Shop_frame', (x+dx,y-6.15,1.8), (.1,.15,2.7), 'white')
    box('Shop_sign', (x,y-6.20,3.55), (23,.20,.8), 'blue' if sign != 'АПТЕКА' else 'leaf3')
    label(sign, (x,y-6.33,3.32), .54)


def tree(x,y,height=8):
    cylinder('Tree_bed', (x,y,.03), 1.3,.08,'soil',12)
    cylinder('Tree_trunk', (x,y,height*.36), .17,height*.72,'wood',8)
    for k in range(4):
        xx = x+random.uniform(-1.25,1.25)
        yy = y+random.uniform(-1.25,1.25)
        key = 'leaf_mesh'
        if key not in cache:
            bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1)
            tmp = bpy.context.object
            cache[key] = tmp.data
            bpy.data.objects.remove(tmp, do_unlink=True)
        mesh = cache[key]
        # Keep shared geometry; material slots differ per object.
        if not mesh.materials:
            mesh.materials.append(materials['leaf1'])
        ob = bpy.data.objects.new('Tree_crown', mesh)
        scene.collection.objects.link(ob)
        ob.location = (xx,yy,height*.73+random.uniform(-.5,1.1))
        ob.scale = (2.15,2.1,2.6)
        ob.material_slots[0].link = 'OBJECT'
        ob.material_slots[0].material = materials[random.choice(('leaf1','leaf2','leaf3'))]


def bench(x,y):
    for dx in (-.8,.8):
        box('Bench_support', (x+dx,y,.25), (.16,.7,.5), 'metal')
    for dy in (-.24,0,.24):
        box('Bench_slat', (x,y+dy,.55), (2.25,.18,.1), 'wood')
    for z in (.85,1.1):
        box('Bench_back', (x,y+.32,z), (2.25,.10,.15), 'wood')


for x in range(-126,132,19):
    if abs(x) < 22:
        continue
    for y in (-12,12):
        tree(x,y,random.uniform(7,9))
for x,y in [(-112,89),(-92,89),(-60,89),(-35,88), (30,87), (30,65),
            (-96,-93),(-74,-94),(-49,-93), (31,-89),(81,-94),(123,-95),
            (35,47),(70,48),(89,49),(-102,-45),(-76,-44),(-49,-43)]:
    tree(x,y,random.uniform(7,10))
for x in range(-126,132,28):
    for y in (-101,101):
        tree(x,y,8)
for x,y in ((37,45),(64,45),(-97,-46),(-67,-46),(-43,87)):
    bench(x,y)
# Court playground: sand, slide, swing and a small sports enclosure.
box('Playground_surface', (61,54,.02), (23,13,.08), 'playground')
box('Sandpit_border', (55,54,.18), (5,4,.3), 'wood')
box('Sand', (55,54,.35), (4.6,3.6,.12), 'cream')
for x in (64,69):
    for y in (51,55):
        cylinder('Swing_post', (x,y,1.65), .07,3.3,'blue')
box('Swing_beam', (66.5,53,3.3), (5.3,.14,.14), 'blue')
for x in (65.5,67.5):
    for dx in (-.3,.3):
        cylinder('Swing_chain', (x+dx,53,2), .017,2.5,'metal',6)
    box('Swing_seat', (x,53,.75), (.8,.4,.08), 'yellow')
# Street lighting and pedestrian signs.
for x in range(-124,130,32):
    for y in (-10.1,10.1):
        if abs(x) < 22:
            continue
        cylinder('Streetlight_pole', (x,y,4), .085,8,'metal')
        box('Streetlight_arm', (x,y+(-.7 if y>0 else .7),7.9), (.12,1.5,.12), 'metal')
        box('Streetlight_head', (x,y+(-1.4 if y>0 else 1.4),7.85), (.45,.8,.14), 'roof')
for x,y in ((-11,-20),(11,20),(-22,11),(22,-11)):
    cylinder('Crossing_sign_pole', (x,y,1.7), .045,3.4,'metal')
    box('Crossing_sign', (x,y,3), (.8,.08,.8), 'blue')
    box('Crossing_sign_inset', (x,y-.05,3), (.57,.025,.57), 'white')
# Bollards protect corners but leave pedestrian crossings and driveways open.
for x,y in ((-9,-9),(-9,9),(9,-9),(9,9)):
    cylinder('Bollard', (x,y,.43), .11,.86,'metal')

# Convert signs to geometry for a self-contained portable export.
bpy.ops.object.select_all(action='DESELECT')
for ob in scene.objects:
    if ob.type == 'FONT':
        ob.select_set(True)
if bpy.context.selected_objects:
    bpy.context.view_layer.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.convert(target='MESH')
bpy.ops.object.select_all(action='DESELECT')
scene.world.use_nodes = True
world_nodes = scene.world.node_tree.nodes
world_nodes.get('Background').inputs['Color'].default_value = (.35,.48,.68,1)
world_nodes.get('Background').inputs['Strength'].default_value = .5
light_data = bpy.data.lights.new('Sun', 'SUN')
light_data.energy, light_data.angle = 2.5, math.radians(8)
sun = bpy.data.objects.new('Sun',light_data)
scene.collection.objects.link(sun)
sun.rotation_euler = (math.radians(30),math.radians(-20),math.radians(-35))
cam_data = bpy.data.cameras.new('Preview_camera')
cam = bpy.data.objects.new('Preview_camera',cam_data)
scene.collection.objects.link(cam)
scene.camera = cam
scene.render.engine = 'CYCLES'
scene.cycles.samples = 64
# Some Linux Blender builds omit OpenImageDenoise.
scene.cycles.use_denoising = False
scene.render.resolution_x, scene.render.resolution_y = 1500, 1000
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
scene.view_settings.view_transform = 'AgX'


def camera(position, target, lens):
    cam.location = position
    cam.rotation_euler = (Vector(target)-cam.location).to_track_quat('-Z','Y').to_euler()
    cam.data.lens = lens


camera((-212,-236,190), (-8,15,0), 40)
mesh_objects = [o for o in scene.objects if o.type == 'MESH']
stats = {'objects': len(mesh_objects), 'materials': len(materials),
         'triangles': sum(sum(len(p.vertices)-2 for p in ob.data.polygons) for ob in mesh_objects),
         'extent_metres': [420,340], 'source': 'Original procedural geometry; no Black Russia assets',
         'status': 'Preview only. Not imported into Unreal or measured on Android.'}
stats.update(export_batched_glb(mesh_objects, OUT/'CityLive_PoliceDistrict.glb'))
(OUT/'GeometryStats.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CityLive_PoliceDistrict.blend'))
for name,position,target,lens in (
    ('Overview',(-212,-236,190),(-8,15,0),40),
    ('Police',(-129,3,16),(-78,56,5),40),
    ('Street',(10,-36,4),(-70,59,5),32)):
    camera(position,target,lens)
    scene.render.filepath = str(OUT/(name+'.png'))
    bpy.ops.render.render(write_still=True)
print('CITYLIVE_POLICE_PREVIEW '+json.dumps(stats))
