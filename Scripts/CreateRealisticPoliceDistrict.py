"""Textured Blender style revision. Existing game maps are not changed."""
import bpy
import json
import math
from mathutils import Vector
from pathlib import Path
import random
import runpy
import sys

OUT = Path(sys.argv[sys.argv.index('--')+1]).resolve()
script = Path(__file__).resolve().parent
sys.path.insert(0,str(script))
from CityLawnMaterial import realistic_lawn, grass_blade_variation
from CitySurfaceDetail import apply_surface_detail
sys.argv = ['blender', '--', str(OUT), '--skip-render', '--skip-export']
base = runpy.run_path(str(script/'CreatePoliceDistrictBlender.py'))
scene, mats = bpy.context.scene, base['materials']
box, cylinder, material = base['box'], base['cylinder'], base['material']
random.seed(20261031)
textures = OUT/'Textures'


def pbr(name, asset, tile):
    mat = mats[name]
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    shader = nodes.get('Principled BSDF')
    for role, suffix, slot in (('color','diff','Base Color'), ('rough','rough','Roughness'),
                              ('normal','nor_gl','Normal')):
        path = textures/(asset+'_'+suffix+'_1k.jpg')
        if not path.is_file():
            raise RuntimeError('Missing CC0 texture: '+str(path))
        image = bpy.data.images.load(str(path),check_existing=True)
        image.filepath = '//Textures/'+path.name
        if role != 'color':
            image.colorspace_settings.name = 'Non-Color'
        node = nodes.new('ShaderNodeTexImage')
        node.image = image
        if role == 'normal':
            normal = nodes.new('ShaderNodeNormalMap')
            normal.inputs['Strength'].default_value = .45 if name == 'police_wall' else .7
            links.new(node.outputs['Color'],normal.inputs['Color'])
            links.new(normal.outputs['Normal'],shader.inputs[slot])
        else:
            links.new(node.outputs['Color'],shader.inputs[slot])
    return tile


tiles = {name:pbr(name,asset,tile) for name,asset,tile in (
    ('asphalt','asphalt_02',4), ('police_wall','grey_plaster_03',3),
    ('cream','grey_plaster_03',3), ('brick','red_brick_03',3),
    ('brick_light','red_brick_03',3), ('concrete','concrete_pavement',3),
    ('paving','concrete_pavement',3), ('grass','grass_ground',4))}
# Real-world planar UVs per face; retained in spatial export batches.
seen = set()
for obj in list(scene.objects):
    if obj.type != 'MESH' or obj.data in seen:
        continue
    mesh = obj.data
    seen.add(mesh)
    name = obj.material_slots[0].material.name
    if name not in tiles:
        continue
    mesh.update()
    uv = mesh.uv_layers.new(name='UVMap')
    for polygon in mesh.polygons:
        axis = max(range(3), key=lambda i:abs(polygon.normal[i]))
        a,b = ((1,2),(0,2),(0,1))[axis]
        for index in polygon.loop_indices:
            point = mesh.vertices[mesh.loops[index].vertex_index].co
            uv.data[index].uv = (point[a]/tiles[name],point[b]/tiles[name])

glass = mats['glass'].node_tree.nodes.get('Principled BSDF')
glass.inputs['Base Color'].default_value = (.026,.042,.052,1)
glass.inputs['Metallic'].default_value = .65
glass.inputs['Roughness'].default_value = .13
material('curtain',(.33,.31,.26),roughness=.95)
material('curtain_dark',(.08,.095,.10),roughness=.95)
material('bark',(.56,.56,.47))
material('bark_dark',(.07,.065,.045))
material('LeafSprig',(.18,.26,.09),roughness=.8)
leaf_mat = mats['LeafSprig']
for role, ext, slot in (('diff','jpg','Base Color'), ('alpha','png','Alpha')):
    image = bpy.data.images.load(str(textures/('tree_small_02_leaves_'+role+'_1k.'+ext)),check_existing=True)
    image.filepath = '//Textures/'+Path(image.filepath).name
    if role == 'alpha':
        image.colorspace_settings.name = 'Non-Color'
    node = leaf_mat.node_tree.nodes.new('ShaderNodeTexImage')
    node.image = image
    leaf_mat.node_tree.links.new(node.outputs['Color'],leaf_mat.node_tree.nodes.get('Principled BSDF').inputs[slot])
# Vary the window interiors instead of repeating identical blue rectangles.
for obj in list(scene.objects):
    if obj.name.startswith('Window_glass') and random.random()<.35:
        x,y,z = obj.location
        shade = 'curtain' if random.random()<.55 else 'curtain_dark'
        box('Inner_blind',(x-.27,y-.029,z),(.65,.015,1.3),shade)
        if random.random()<.25:
            box('Window_AC',(x+.95,y-.36,z-.95),(.64,.45,.45),'white')

# Add parapets, roof flashings, rainwater pipes, entrance fixtures and meters.
for cx,cy,w,d,h in ((-78,62,58,28,9.3),(77,73,80,17,15.5),(-79,-68,80,17,15.5),
                     (54,-67,27,23,27.9),(104,-72,28,23,27.9)):
    for side in (-1,1):
        box('Roof_parapet',(cx,cy+side*d/2,h+.65),(w+.4,.32,.65),'police_wall')
        box('Roof_flashing',(cx,cy+side*d/2,h+1.02),(w+.65,.48,.065),'metal')
    for side in (-1,1):
        x = cx+side*(w/2-.65)
        cylinder('Rain_pipe',(x,cy-d/2-.19,h/2),.10,h,'metal')
        for z in range(1,int(h),3):
            cylinder('Pipe_mount',(x,cy-d/2-.19,z),.13,.06,'metal')
for x in (-82,-74):
    box('Entrance_luminaire',(x,45.3,4.35),(.45,.2,.14),'white')
for x in (-89,-67):
    box('Wall_notice_board',(x,47.70,1.7),(1.1,.15,1.4),'blue')
    box('Notice_paper',(x,47.60,1.7),(.78,.02,1.1),'white')

# Replace geometric ball crowns with branches and individual small leaves.
trees = [obj for obj in scene.objects if obj.name.startswith('Tree_trunk')]
for trunk in trees:
    # Perimeter trees used to stand in the outer sidewalk strip. Put them in
    # the adjoining lawn before generating their branches and foliage.
    if abs(trunk.location.y) > 98:
        trunk.location.y = math.copysign(95,trunk.location.y)
    trunk.location.z -= .04  # Trunk base enters the ground, without a visible gap.
for obj in list(scene.objects):
    if obj.name.startswith(('Tree_crown','Tree_bed')):
        bpy.data.objects.remove(obj,do_unlink=True)
for trunk in trees:
    x,y,z = trunk.location
    height = trunk.dimensions.z/.72
    trunk.data = trunk.data.copy()
    trunk.data.materials.clear()
    trunk.data.materials.append(mats['bark'])
    clusters = []
    for j in range(8):
        angle = random.uniform(0,math.tau)
        end = Vector((x+math.cos(angle)*random.uniform(1,2.8),
                      y+math.sin(angle)*random.uniform(1,2.8),height*random.uniform(.65,1.02)))
        start = Vector((x,y,height*random.uniform(.35,.55)))
        delta = end-start
        branch = cylinder('Birch_branch',(start+end)*.5,.035,delta.length,'bark',6)
        branch.rotation_euler = delta.to_track_quat('Z','Y').to_euler()
        clusters.append(end)
    for group in range(3):
        verts, faces = [], []
        for k in range(64):
            center = random.choice(clusters)
            # Clustered leaves form an irregular, permeable canopy.
            offset = Vector((random.gauss(0,.8),random.gauss(0,.8),random.gauss(0,.7)))
            pos = center+offset
            angle = random.uniform(0,math.tau)
            length = random.uniform(.18,.30)
            a = Vector((math.cos(angle)*length,math.sin(angle)*length,0))
            b = Vector((random.uniform(-.2,.2),random.uniform(-.2,.2),random.uniform(.4,.7)))
            n = len(verts)
            verts.extend(tuple(v) for v in (pos-a-b,pos+a-b,pos+a+b,pos-a+b))
            faces.append((n,n+1,n+2,n+3))
        mesh = bpy.data.meshes.new('Birch_leaves')
        mesh.from_pydata(verts,[],faces)
        mesh.materials.append(leaf_mat)
        uv = mesh.uv_layers.new(name='UVMap')
        for polygon in mesh.polygons:
            for loop, value in zip(polygon.loop_indices,((0,0),(.44,0),(.44,1),(0,1))):
                uv.data[loop].uv = value
        leaf = bpy.data.objects.new('Birch_leaves',mesh)
        scene.collection.objects.link(leaf)
        # World-space vertices, object origin at tree for spatial batching.
        leaf.location = (x,y,0)
        for vertex in mesh.vertices:
            vertex.co -= Vector((x,y,0))
    for j in range(9):
        patch = box('Birch_bark_mark',(x+.17,y,random.uniform(.3,height*.6)),
                    (.025,random.uniform(.1,.25),random.uniform(.04,.13)),'bark_dark')

# A few weeds at curb bases rather than a uniformly clean green carpet.
for j in range(160):
    x = random.uniform(-125,-25)
    y = random.choice((10.5,13.5))+random.uniform(-.15,.15)
    ob = box('Curb_weed',(x,y,.12),(.025,.17,random.uniform(.1,.25)),'leaf3',random.uniform(0,math.tau))

# Green lawns with sparse grass tufts. Keep the surface continuous under trees.
realistic_lawn(mats['grass'])
verts, faces = [], []
patches = [(-124,-90,35,43),(-67,-29,35,43),(-125,-25,83,95),
           (-106,-36,-53,-37),(30,44,43,60),(80,105,43,59),
           (31,121,-48,-39),(-125,125,12,14),(-125,125,-14,-12)]
for patch in patches:
    xmin,xmax,ymin,ymax = patch
    for j in range(150):
        x,y = random.uniform(xmin,xmax),random.uniform(ymin,ymax)
        if abs(x) < 12:  # The north/south street remains clear.
            continue
        for blade in range(random.randint(5,9)):
            angle = random.uniform(0,math.tau)
            px,py = x+random.uniform(-.13,.13),y+random.uniform(-.13,.13)
            width,height = random.uniform(.015,.035),random.uniform(.09,.22)
            dx,dy = math.cos(angle)*width,math.sin(angle)*width
            i = len(verts)
            verts.extend(((px-dx,py-dy,-.02),(px+dx,py+dy,-.02),
                          (px+math.sin(angle)*.04,py-math.cos(angle)*.04,height)))
            faces.append((i,i+1,i+2))
mesh = bpy.data.meshes.new('Sparse_grass_tufts')
mesh.from_pydata(verts,[],faces)
mesh.materials.append(mats['leaf2'])
tufts = bpy.data.objects.new('Sparse_grass_tufts',mesh)
scene.collection.objects.link(tufts)
grass_blade_variation(tufts)
apply_surface_detail(scene,textures)

# Daylight: physical sky for reflections, with its sun contribution reduced.
world = scene.world.node_tree
sky = world.nodes.new('ShaderNodeTexSky')
sky.sky_type = 'NISHITA'
sky.sun_elevation = math.radians(38)
sky.sun_rotation = math.radians(130)
sky.sun_disc = False
world.links.new(sky.outputs['Color'],world.nodes.get('Background').inputs['Color'])
world.nodes.get('Background').inputs['Strength'].default_value = .12
scene.view_settings.exposure = -.5
scene.cycles.samples = 80
scene.render.resolution_x, scene.render.resolution_y = 1600,1000
mesh_objects = [ob for ob in scene.objects if ob.type == 'MESH']
stats = {'objects':len(mesh_objects),
         'triangles':sum(sum(len(p.vertices)-2 for p in ob.data.polygons) for ob in mesh_objects),
         'status':'Blender style preview, no Android performance claim',
         'textures':'Poly Haven CC0; see TextureCredits.json'}
# Keep source images externally as portable relative paths next to the blend.
base['camera']((-26,-24,7),(-82,-65,7),35)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'CityLive_PoliceDistrict_Realistic.blend'))
(OUT/'GeometryStats.json').write_text(json.dumps(stats,indent=2),encoding='utf-8')
for name,position,target,lens in (
    ('HomesRealistic',(-26,-24,7),(-82,-65,7),35),
    ('CourtyardRealistic',(36,38,4.5),(79,73,7),32)):
    base['camera'](position,target,lens)
    if name == 'CourtyardRealistic':
        scene.cycles.samples = 64
    scene.render.filepath = str(OUT/(name+'.png'))
    bpy.ops.render.render(write_still=True)
print('CITYLIVE_REALISTIC_PREVIEW '+json.dumps(stats))
