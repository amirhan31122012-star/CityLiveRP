"""Bake the approved Blender scene into portable FBX + PBR textures for Unreal.

blender -b APPROVED.blend -t 6 --python Scripts/BakePoliceDistrict.py -- OUTPUT
"""
import bpy
import hashlib
import json
import math
import numpy as np
from pathlib import Path
import shutil
import sys

sys.path.insert(0,str(Path(__file__).parent))
from PoliceDistrictPolicy import collision_kind

out = Path(sys.argv[sys.argv.index('--')+1]).resolve()
out.mkdir(parents=True,exist_ok=True)
(out/'Textures').mkdir(exist_ok=True)
original = bpy.context.scene
source_objects = [o for o in original.objects if o.type=='MESH']
used = sorted({o.material_slots[0].material for o in source_objects},key=lambda m:m.name)
source_tiles = {'asphalt':4,'police_wall':3,'cream':3,'brick':3,'brick_light':3,
                'concrete':3,'paving':3,'grass':4}
world_tiles = {'grass':8,'playground':2,'LawnBlades':2,'SandGranules':1}
records, by_material, texture_cache = [], {}, {}
reuse={}
if '--reuse-materials' in sys.argv:
    previous=json.loads((out/'manifest.json').read_text(encoding='utf-8'))
    if previous['source_blend_sha256']!=hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest():
        raise RuntimeError('Source changed; bake materials again')
    reuse={r['source_name']:r for r in previous['materials']}
bake_scene = bpy.data.scenes.new('DistrictBake')
bake_scene.render.engine='CYCLES'
bake_scene.cycles.samples=1
bake_scene.cycles.use_denoising=False
quad = bpy.data.meshes.new('BakeQuad')
quad.from_pydata([(0,0,0),(1,0,0),(1,1,0),(0,1,0)],[],[(0,1,2,3)])
quad.uv_layers.new(name='SourceUV')
quad.uv_layers.new(name='BakeUV')
plane = bpy.data.objects.new('BakeQuad',quad)
bake_scene.collection.objects.link(plane)


def texture_info(path):
    return {'file':str(path.relative_to(out)),'uv':0,'scale':[1,1],'offset':[0,0],'rotation':0}


def copy_texture(image):
    source=Path(bpy.path.abspath(image.filepath)).resolve()
    dest=out/'Textures'/source.name
    if not source.is_file(): raise RuntimeError('Missing source texture '+str(source))
    if source!=dest: shutil.copyfile(source,dest)
    return texture_info(dest)


def save_image(image,path):
    image.filepath_raw=str(path)
    image.file_format='PNG'
    image.save()
    return texture_info(path)


def bake(material, channel, slot, tile, resolution):
    print('CITYLIVE_BAKE '+material.name+' '+channel,flush=True)
    bpy.context.window.scene=bake_scene
    for i,vertex in enumerate(quad.vertices):
        vertex.co=((0,0,0),(tile,0,0),(tile,tile,0),(0,tile,0))[i]
    for layer in quad.uv_layers:
        factor=tile/source_tiles.get(material.name,tile) if layer.name=='SourceUV' else 1
        for loop,uv in zip(layer.data,((0,0),(factor,0),(factor,factor),(0,factor))): loop.uv=uv
    quad.uv_layers['SourceUV'].active_render=True
    quad.uv_layers.active=quad.uv_layers['BakeUV']
    mat=material.copy()
    quad.materials.clear();quad.materials.append(mat)
    nodes,links=mat.node_tree.nodes,mat.node_tree.links
    uv=nodes.new('ShaderNodeUVMap');uv.uv_map='SourceUV'
    for node in list(nodes):
        if node.type=='TEX_IMAGE' and not node.inputs['Vector'].is_linked:
            links.new(uv.outputs['UV'],node.inputs['Vector'])
        if node.type=='NORMAL_MAP': node.uv_map='SourceUV'
    shader=nodes.get('Principled BSDF')
    if channel!='Normal':
        emission=nodes.new('ShaderNodeEmission')
        socket=shader.inputs[channel]
        if socket.is_linked: links.new(socket.links[0].from_socket,emission.inputs['Color'])
        else:
            value=socket.default_value
            emission.inputs['Color'].default_value=tuple(value) if channel=='Base Color' else (value,value,value,1)
        output=next(n for n in nodes if n.type=='OUTPUT_MATERIAL' and n.is_active_output)
        links.new(emission.outputs[0],output.inputs['Surface'])
    image=bpy.data.images.new('Baked_'+slot+'_'+channel,width=resolution,height=resolution,alpha=True)
    image.colorspace_settings.name='sRGB' if channel=='Base Color' else 'Non-Color'
    target=nodes.new('ShaderNodeTexImage');target.image=image;nodes.active=target
    bpy.context.view_layer.objects.active=plane;plane.select_set(True)
    bpy.ops.object.bake(type='NORMAL' if channel=='Normal' else 'EMIT',
                        normal_space='TANGENT',margin=4,use_clear=True,uv_layer='BakeUV')
    quad.materials.clear();bpy.data.materials.remove(mat)
    bpy.context.window.scene=original
    return image


for index,material in enumerate(used):
    print('CITYLIVE_MATERIAL '+str(index+1)+'/'+str(len(used))+' '+material.name,flush=True)
    if reuse:
        record=reuse[material.name]
        records.append(record);by_material[material]=record
        continue
    shader=material.node_tree.nodes.get('Principled BSDF')
    slot='CLPD_'+str(index).zfill(3)
    tile=world_tiles.get(material.name,source_tiles.get(material.name,1))
    resolution=2048 if material.name=='grass' else 1024
    record={'slot':slot,'source_name':material.name,'base':None,'normal':None,'packed':None,
            'emissive':None,'emissive_factor':[0,0,0],'base_factor':list(shader.inputs['Base Color'].default_value),
            'roughness':float(shader.inputs['Roughness'].default_value),
            'metallic':float(shader.inputs['Metallic'].default_value),'alpha':'OPAQUE','cutoff':.45,
            'two_sided':material.name in ('LeafSprig','LawnBlades','leaf1','leaf2','leaf3')}
    base_socket=shader.inputs['Base Color']
    if shader.inputs['Alpha'].is_linked:
        color=base_socket.links[0].from_node.image
        alpha=shader.inputs['Alpha'].links[0].from_node.image
        w,h=color.size
        rgba=np.empty(w*h*4,dtype=np.float32);mask=np.empty(w*h*4,dtype=np.float32)
        color.pixels.foreach_get(rgba);alpha.pixels.foreach_get(mask)
        rgba.reshape(-1,4)[:,3]=mask.reshape(-1,4)[:,0]
        image=bpy.data.images.new('LeafRGBA',width=w,height=h,alpha=True)
        image.pixels.foreach_set(rgba);image.update()
        record['base']=save_image(image,out/'Textures'/(slot+'_Color.png'))
        bpy.data.images.remove(image)
        record['base_factor']=[1,1,1,1];record['alpha']='MASK'
    elif base_socket.is_linked:
        node=base_socket.links[0].from_node
        if node.type=='TEX_IMAGE': record['base']=copy_texture(node.image)
        else:
            image=bake(material,'Base Color',slot,tile,resolution)
            record['base']=save_image(image,out/'Textures'/(slot+'_Color.png'))
            bpy.data.images.remove(image)
        record['base_factor']=[1,1,1,1]
    if shader.inputs['Normal'].is_linked:
        image=bake(material,'Normal',slot,tile,resolution)
        record['normal']=save_image(image,out/'Textures'/(slot+'_Normal.png'))
        bpy.data.images.remove(image)
    if shader.inputs['Roughness'].is_linked:
        image=bake(material,'Roughness',slot,tile,resolution)
        raw=np.empty(resolution*resolution*4,dtype=np.float32);image.pixels.foreach_get(raw)
        raw=raw.reshape(-1,4)
        packed=np.ones_like(raw);packed[:,1]=raw[:,0];packed[:,2]=record['metallic']
        target=bpy.data.images.new('Packed',width=resolution,height=resolution,alpha=True)
        target.colorspace_settings.name='Non-Color';target.pixels.foreach_set(packed.ravel());target.update()
        record['packed']=save_image(target,out/'Textures'/(slot+'_Data.png'))
        record['roughness']=1;record['metallic']=1
        bpy.data.images.remove(image);bpy.data.images.remove(target)
    records.append(record);by_material[material]=record

# Spatial batches, with separate collision classes. Do not make leaves, paint
# or grass block movement, and keep the police building identifiable for spawn.
bpy.context.window.scene=original
bpy.context.view_layer.update()
groups={}
for obj in source_objects:
    material=obj.material_slots[0].material
    record=by_material[material]
    vertices=[obj.matrix_world@v.co for v in obj.data.vertices]
    kind=collision_kind(obj.name)
    layer=obj.data.uv_layers.active
    for poly in obj.data.polygons:
        center=sum((vertices[i] for i in poly.vertices),vertices[poly.vertices[0]]*0)/len(poly.vertices)
        key=(kind,math.floor(center.x/32),math.floor(center.y/32),record['slot'])
        v,f,uv=groups.setdefault(key,([],[],[]))
        start=len(v);v.extend(tuple(vertices[i]) for i in poly.vertices)
        f.append(tuple(range(start,start+len(poly.vertices))))
        normal=obj.matrix_world.to_3x3()@poly.normal
        axis=max(range(3),key=lambda i:abs(normal[i]));a,b=((1,2),(0,2),(0,1))[axis]
        for loop in poly.loop_indices:
            pos=vertices[obj.data.loops[loop].vertex_index]
            if material.name in world_tiles:
                scale=world_tiles[material.name];uv.append((pos[a]/scale,pos[b]/scale))
            elif layer: uv.append(tuple(layer.data[loop].uv))
            else:
                scale=source_tiles.get(material.name,1);uv.append((pos[a]/scale,pos[b]/scale))
fbx_materials={}
for record in records:
    material=bpy.data.materials.new(record['slot']);material.diffuse_color=tuple(record['base_factor'])
    fbx_materials[record['slot']]=material
bpy.ops.object.select_all(action='DESELECT')
batch_objects=[]
for (kind,x,y,slot),(vertices,faces,coords) in groups.items():
    cell=lambda v:('m'+str(-v)) if v<0 else ('p'+str(v))
    name='PD_'+kind+'_x'+cell(x)+'_y'+cell(y)+'_'+slot
    mesh=bpy.data.meshes.new(name);mesh.from_pydata(vertices,[],faces);mesh.materials.append(fbx_materials[slot])
    uv=mesh.uv_layers.new(name='UVMap')
    for loop,value in zip(uv.data,coords):loop.uv=value
    obj=bpy.data.objects.new(name,mesh);original.collection.objects.link(obj);obj.select_set(True);batch_objects.append(obj)
bpy.ops.export_scene.fbx(filepath=str(out/'PoliceDistrict.fbx'),use_selection=True,object_types={'MESH'},
                          use_triangles=True,use_tspace=True,mesh_smooth_type='FACE',
                          axis_forward='-Y',axis_up='Z',apply_unit_scale=True,bake_anim=False,add_leaf_bones=False)
files=[out/'PoliceDistrict.fbx']+sorted((out/'Textures').iterdir())
manifest={'version':1,'district':'PoliceDistrict','fbx':'PoliceDistrict.fbx','materials':records,
          'triangles':sum(sum(len(p.vertices)-2 for p in o.data.polygons) for o in batch_objects),
          'mesh_count':len(batch_objects),'source_blend_sha256':hashlib.sha256(Path(bpy.data.filepath).read_bytes()).hexdigest(),
          'files':[{'file':str(p.relative_to(out)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()} for p in files]}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
print('CITYLIVE_DISTRICT_PACKAGE_READY '+json.dumps({k:manifest[k] for k in ('triangles','mesh_count')}),flush=True)
