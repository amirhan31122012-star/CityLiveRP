"""Blender surface study: tactile rubber, ground and masonry, not game settings."""
from pathlib import Path
import math
import random
import bpy


def reset(material):
    nodes, links = material.node_tree.nodes, material.node_tree.links
    for n in list(nodes):
        if n.name.startswith('CitySurface_'):
            nodes.remove(n)
    def add(kind, name):
        n = nodes.new(kind)
        n.name = 'CitySurface_'+name
        return n
    return nodes, links, nodes.get('Principled BSDF'), add


def height_detail(material, asset, folder, distance, strength):
    nodes, links, shader, add = reset(material)
    image = bpy.data.images.load(str(Path(folder)/(asset+'_disp_1k.png')),check_existing=True)
    image.filepath = '//Textures/'+Path(image.filepath).name
    image.colorspace_settings.name = 'Non-Color'
    tex = add('ShaderNodeTexImage','HeightMap')
    tex.image = image
    bump = add('ShaderNodeBump','Relief')
    bump.inputs['Distance'].default_value = distance
    bump.inputs['Strength'].default_value = strength
    links.new(tex.outputs['Color'],bump.inputs['Height'])
    normal = next((n for n in nodes if n.type == 'NORMAL_MAP'),None)
    if normal:
        normal.inputs['Strength'].default_value = .9
        links.new(normal.outputs['Normal'],bump.inputs['Normal'])
    links.new(bump.outputs['Normal'],shader.inputs['Normal'])
    rough = next((n for n in nodes if n.type == 'TEX_IMAGE' and n.image and '_rough_' in n.image.name),None)
    if rough:
        remap = add('ShaderNodeMapRange','RoughnessRange')
        remap.inputs['To Min'].default_value = .72
        remap.inputs['To Max'].default_value = .98
        links.new(rough.outputs['Color'],remap.inputs['Value'])
        links.new(remap.outputs['Result'],shader.inputs['Roughness'])


def rubber(material):
    nodes, links, shader, add = reset(material)
    geometry = add('ShaderNodeNewGeometry','Position')
    tiles = add('ShaderNodeTexBrick','RubberTiles')
    tiles.offset = 0
    for key,value in (('Scale',1),('Mortar Size',.0025),('Mortar Smooth',.002),
                      ('Brick Width',.5),('Row Height',.5)):
        tiles.inputs[key].default_value = value
    tiles.inputs['Color1'].default_value = (.32,.12,.055,1)
    tiles.inputs['Color2'].default_value = (.40,.17,.082,1)
    tiles.inputs['Mortar'].default_value = (.12,.055,.025,1)
    links.new(geometry.outputs['Position'],tiles.inputs['Vector'])
    grain = add('ShaderNodeTexNoise','RubberGranules')
    grain.inputs['Scale'].default_value = 140
    grain.inputs['Detail'].default_value = 2
    links.new(geometry.outputs['Position'],grain.inputs['Vector'])
    color = add('ShaderNodeMixRGB','GrainColor')
    color.blend_type = 'MULTIPLY'
    color.inputs[0].default_value = .35
    links.new(tiles.outputs['Color'],color.inputs[1])
    links.new(grain.outputs['Fac'],color.inputs[2])
    links.new(color.outputs['Color'],shader.inputs['Base Color'])
    joint = add('ShaderNodeBump','RecessedJoints')
    joint.invert = True
    joint.inputs['Distance'].default_value = .007
    joint.inputs['Strength'].default_value = .65
    links.new(tiles.outputs['Fac'],joint.inputs['Height'])
    bump = add('ShaderNodeBump','GrainRelief')
    bump.inputs['Distance'].default_value = .008
    bump.inputs['Strength'].default_value = .55
    links.new(grain.outputs['Fac'],bump.inputs['Height'])
    links.new(joint.outputs['Normal'],bump.inputs['Normal'])
    links.new(bump.outputs['Normal'],shader.inputs['Normal'])
    rough = add('ShaderNodeMapRange','MatteRoughness')
    rough.inputs['To Min'].default_value = .8
    rough.inputs['To Max'].default_value = .98
    links.new(grain.outputs['Fac'],rough.inputs['Value'])
    links.new(rough.outputs['Result'],shader.inputs['Roughness'])


def sand_surface(scene):
    obj = scene.objects.get('Sand')
    if not obj:
        return
    border = scene.objects.get('Sandpit_border')
    if border:
        # The original solid box hid the sand after lowering its surface.
        # Replace it with four actual boards, leaving the centre open.
        cx,cy,cz = border.location
        bpy.data.objects.remove(border,do_unlink=True)
        for i,(dx,dy,sx,sy) in enumerate(((0,-1.92,5,.16),(0,1.92,5,.16),
                                          (-2.42,0,.16,3.68),(2.42,0,.16,3.68))):
            x,y,z = sx/2,sy/2,.15
            mesh = bpy.data.meshes.new('Sandbox_board_'+str(i))
            mesh.from_pydata([(-x,-y,-z),(x,-y,-z),(x,y,-z),(-x,y,-z),
                              (-x,-y,z),(x,-y,z),(x,y,z),(-x,y,z)],[],
                             [(0,3,2,1),(4,5,6,7),(0,1,5,4),(1,2,6,5),(2,3,7,6),(3,0,4,7)])
            mesh.materials.append(bpy.data.materials['wood'])
            board = bpy.data.objects.new(mesh.name,mesh)
            scene.collection.objects.link(board)
            board.location = (cx+dx,cy+dy,cz)
    material = bpy.data.materials.get('SandGranules')
    if not material:
        material = bpy.data.materials.new('SandGranules')
        material.use_nodes = True
    obj.data = obj.data.copy()
    obj.data.materials[0] = material
    obj.location.z = .22  # Sand sits below the surrounding wooden lip.
    nodes, links, shader, add = reset(material)
    shader.inputs['Base Color'].default_value = (.48,.36,.18,1)
    shader.inputs['Roughness'].default_value = .93
    geo = add('ShaderNodeNewGeometry','SandPosition')
    noise = add('ShaderNodeTexNoise','SandGrains')
    noise.inputs['Scale'].default_value = 230
    links.new(geo.outputs['Position'],noise.inputs['Vector'])
    bump = add('ShaderNodeBump','SandRelief')
    bump.inputs['Distance'].default_value = .004
    bump.inputs['Strength'].default_value = .45
    links.new(noise.outputs['Fac'],bump.inputs['Height'])
    links.new(bump.outputs['Normal'],shader.inputs['Normal'])


def short_grass(scene):
    if any(o.name.startswith('Fine_lawn_grass_') for o in scene.objects):
        return
    rng = random.Random(44031)
    for index, patch in enumerate(((30,44,43,60),(80,105,43,59),(-106,-36,-53,-37),
                                    (-124,-90,35,43),(-67,-29,35,43))):
        xmin,xmax,ymin,ymax = patch
        verts, faces = [], []
        for j in range(450):
            x,y = rng.uniform(xmin,xmax),rng.uniform(ymin,ymax)
            for blade in range(4):
                angle = rng.uniform(0,math.tau)
                width = rng.uniform(.009,.018)
                px,py = x+rng.uniform(-.06,.06),y+rng.uniform(-.06,.06)
                dx,dy = math.cos(angle)*width,math.sin(angle)*width
                i = len(verts)
                verts.extend(((px-dx,py-dy,-.021),(px+dx,py+dy,-.021),
                              (px+dx*.8,py+dy*.8,rng.uniform(.025,.075))))
                faces.append((i,i+1,i+2))
        mesh = bpy.data.meshes.new('Fine_lawn_grass_'+str(index))
        mesh.from_pydata(verts,[],faces)
        mesh.materials.append(bpy.data.materials['LawnBlades'])
        obj = bpy.data.objects.new(mesh.name,mesh)
        scene.collection.objects.link(obj)


def apply_surface_detail(scene, texture_folder):
    for name,asset,distance,strength in (
        ('police_wall','grey_plaster_03',.012,.65), ('cream','grey_plaster_03',.012,.65),
        ('brick','red_brick_03',.035,.65), ('brick_light','red_brick_03',.035,.65),
        ('grass','grass_ground',.035,.65)):
        height_detail(bpy.data.materials[name],asset,texture_folder,distance,strength)
    rubber(bpy.data.materials['playground'])
    sand_surface(scene)
    short_grass(scene)
    for obj in scene.objects:
        if (obj.name == 'Sparse_grass_tufts' or obj.name.startswith('Fine_lawn_grass_')) and not obj.get('fine_blade_width'):
            # Actual grass blades are millimetres wide; the old broad triangles
            # looked like spikes when viewed near ground level.
            for face in obj.data.polygons:
                a,b,tip = (obj.data.vertices[i] for i in face.vertices)
                center = (a.co+b.co)*.5
                a.co = center+(a.co-center)*.14
                b.co = center+(b.co-center)*.14
                if obj.name == 'Sparse_grass_tufts':
                    tip.co.z = center.z+(tip.co.z-center.z)*.6
            obj['fine_blade_width'] = True
    surface = scene.objects.get('Playground_surface')
    if surface:
        surface.location.z = -.035  # Low transition from lawn to rubber, not a slab.
