"""Face-specific geometry and UV baking. All dimensions are metres, never object scales.

FaceSource.png is an AI-generated texture source; previews must render the meshes.
The original 21-bone bind pose is left intact.
"""
import math
import bpy
import numpy as np
from mathutils import Vector
from mathutils.bvhtree import BVHTree

ROLES = ('Civilian', 'Police', 'Medic', 'Courier')
LANDMARKS = {
    'Civilian':(.499,.119,.405,.579,.684,.87),
    'Police':(.504,.118,.409,.585,.684,.87),
    'Medic':(.499,.119,.399,.586,.687,.895),
    'Courier':(.503,.117,.419,.596,.686,.87),
}


def shape(point, role):
    p = Vector(point)
    if p.z <= 1.59:
        return p
    x, y, z = p
    front = max(0., min(1., (-y-.10)/.10))
    jaw = math.exp(-((z-1.66)/.039)**2)
    cheek = math.exp(-((z-1.718)/.035)**2)
    brow = math.exp(-((z-1.761)/.013)**2)
    settings = {
        'Civilian': (.065, .035, .0010, -.0020),
        'Police': (.18, .07, .0025, -.0035),
        'Medic': (-.015, .045, .0017, .0010),
        'Courier': (.075, .095, -.0007, -.0010),
    }
    jaw_width, cheek_width, brow_depth, chin_lift = settings[role]
    p.x *= 1 + jaw_width*jaw + cheek_width*cheek
    p.y -= front * (brow_depth*brow + .0015*cheek)
    p.z -= chin_lift*jaw*front
    # Remove the tall scan hair silhouette, keeping the forehead and face intact.
    if z > 1.80:
        p.z = 1.80 + (z-1.80)*.63
        p.y += .012*min(1., (z-1.80)/.045)*front
    return p


def eye_centres(role):
    return [shape((sign*.032,-.217,1.745),role) for sign in (-1,1)]


def project(points, role):
    """Match image landmarks to the sculpt's chin, lips, nose, pupils and hairline."""
    centre = eye_centres(role)[1]
    middle,half,eyes,nose,mouth,chin=LANDMARKS[role]
    u = middle + points[:,0] / centre.x * half
    z = points[:,2]
    anchors = [shape((0,-.24,h),role).z for h in (1.59,1.63,1.672,1.704,1.745,1.797,1.90)]
    v = np.interp(z, anchors, (.99,chin,mouth,nose,eyes,.17,.0))
    return np.column_stack((np.clip(u,.01,.99),np.clip(v,.01,.99)))


def _read(path):
    image = bpy.data.images.load(str(path),check_existing=True)
    data = np.empty(image.size[0]*image.size[1]*4,dtype=np.float32)
    image.pixels.foreach_get(data)
    return data.reshape((image.size[1],image.size[0],4))


def skin_tone(root,role):
    atlas=_read(root/'Assets/Characters/Roles/FaceSource.png')
    index=ROLES.index(role);height,width=atlas.shape[0]//2,atlas.shape[1]//2
    panel=atlas[(1-index//2)*height:(2-index//2)*height,(index%2)*width:(index%2+1)*width]
    return panel[int(.40*height):int(.45*height),int(.22*width):int(.28*width),:3].mean(axis=(0,1))


def _write(name,path,data,colour=True):
    image = bpy.data.images.new(name,width=data.shape[1],height=data.shape[0],alpha=False)
    image.colorspace_settings.name='sRGB' if colour else 'Non-Color';image.pixels.foreach_set(data.ravel())
    image.filepath_raw=str(path);image.file_format='PNG';image.save()
    return image


def bake_face(body,role,root,original_uv,indices,size=1024):
    atlas=_read(root/'Assets/Characters/Roles/FaceSource.png')
    base=_read(root/'Assets/Characters/Beige/Image_1.png')
    # Blender image rows start at the bottom; source panels are ordered from top.
    index=ROLES.index(role);half_y,half_x=atlas.shape[0]//2,atlas.shape[1]//2
    row=1-index//2;col=index%2
    source=atlas[row*half_y:(row+1)*half_y,col*half_x:(col+1)*half_x]
    skin_tone=source[int(.40*half_y):int(.45*half_y),int(.22*half_x):int(.28*half_x),:3].mean(axis=(0,1))
    result=np.ones((size,size,4),dtype=np.float32);result[:,:,:3]=skin_tone
    rough=np.ones_like(result);rough[:,:,:3]=.57
    painted=np.zeros((size,size),dtype=bool)
    mesh=body.data;mesh.calc_loop_triangles();uv=mesh.uv_layers.active.data
    coords=np.asarray([v.co[:] for v in mesh.vertices])
    for tri in mesh.loop_triangles:
        if tri.polygon_index not in indices:continue
        points=np.asarray([uv[i].uv[:] for i in tri.loops])*size-.5
        lo=np.maximum(0,np.floor(points.min(axis=0)).astype(int));hi=np.minimum(size-1,np.ceil(points.max(axis=0)).astype(int))
        if np.any(hi<lo):continue
        y,x=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1];a,b,c=points
        d=(b[1]-c[1])*(a[0]-c[0])+(c[0]-b[0])*(a[1]-c[1])
        if abs(d)<1e-8:continue
        u=((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1]))/d
        v=((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1]))/d
        mask=(u>=-.001)&(v>=-.001)&(u+v<=1.001)
        if not mask.any():continue
        x,y,u,v=x[mask],y[mask],u[mask],v[mask]
        bary=np.column_stack((u,v,1-u-v));p=bary@coords[list(tri.vertices)]
        projected=project(p,role)
        sx=np.clip((projected[:,0]*(half_x-1)).astype(int),0,half_x-1)
        sy=np.clip(((1-projected[:,1])*(half_y-1)).astype(int),0,half_y-1)
        face=source[sy,sx,:3]
        old_uv=bary@original_uv[list(tri.loops)]
        bx=((old_uv[:,0]%1)*(base.shape[1]-1)).astype(int);by=((old_uv[:,1]%1)*(base.shape[0]-1)).astype(int)
        old=base[by,bx,:3];lum=old.mean(axis=1)
        fallback=skin_tone[None,:]*np.clip(lum[:,None]/.68,.72,1.12)
        front=np.clip((-p[:,1]-.115)/.08,0,1)*np.clip((.083-np.abs(p[:,0]))/.021,0,1)
        front*=np.clip((p[:,2]-1.605)/.025,0,1)
        # A gradual transition leaves ears/back of neck free from a second face.
        colour=fallback*(1-front[:,None])+face*front[:,None]
        back=np.clip((p[:,1]+.20)/.18,0,1)
        hairline=1.801-.077*back
        hair=np.clip((p[:,2]-hairline)/.025,0,1)*(1-front)
        hair_colour={'Civilian':(.075,.047,.030),'Police':(.13,.09,.058),'Medic':(.22,.215,.20),'Courier':(.24,.11,.051)}[role]
        grain=.92+.035*np.sin(p[:,0]*1050+np.sin(p[:,2]*420)*2)+.025*np.sin(p[:,2]*1200+p[:,1]*840)
        colour=colour*(1-hair[:,None])+np.asarray(hair_colour)[None,:]*grain[:,None]*hair[:,None]
        result[y,x,:3]=np.clip(colour,0,1);painted[y,x]=True
        r=.54+.045*np.sin(p[:,0]*5100+p[:,2]*2700)
        front_hair=np.clip((p[:,2]-(1.798-.013*(np.abs(p[:,0])/.075)**2))/.006,0,1)
        total_hair=np.maximum(hair,front_hair)
        r=r*(1-total_hair)+.94*total_hair
        # Less glossy hair and beard; a little more moisture at the lip line.
        lips=np.exp(-((p[:,2]-shape((0,-.24,1.672),role).z)/.005)**2)*np.exp(-(p[:,0]/.025)**4)*front
        r=r*(1-lips)+.36*lips
        rough[y,x,0]=r
        rough[y,x,1]=.26*(1-total_hair)+.005*total_hair
        rough[y,x,2]=0
    for _ in range(8):
        before=painted.copy()
        for axis,offset in ((0,1),(0,-1),(1,1),(1,-1)):
            neighbour=np.roll(before,offset,axis=axis);fill=neighbour&~painted
            result[fill]=np.roll(result,offset,axis=axis)[fill]
            rough[fill]=np.roll(rough,offset,axis=axis)[fill];painted[fill]=True
    out=root/'Assets/Characters/Roles'
    colour_image=_write('T_Face_'+role,out/('T_Face_'+role+'.png'),result)
    rough_image=_write('T_FaceRough_'+role,out/('T_FaceRough_'+role+'.png'),rough,colour=False)
    material=bpy.data.materials.new('Role_Face_'+role);material.use_nodes=True
    nodes=material.node_tree.nodes;shader=nodes.get('Principled BSDF')
    sample=nodes.new('ShaderNodeTexImage');sample.image=colour_image
    material.node_tree.links.new(sample.outputs['Color'],shader.inputs['Base Color'])
    rough_node=nodes.new('ShaderNodeTexImage');rough_node.image=rough_image
    split=nodes.new('ShaderNodeSeparateColor');split.mode='RGB'
    material.node_tree.links.new(rough_node.outputs['Color'],split.inputs['Color'])
    material.node_tree.links.new(split.outputs['Red'],shader.inputs['Roughness'])
    material.node_tree.links.new(split.outputs['Green'],shader.inputs['Specular IOR Level'])
    body.data.materials.append(material)
    slot=len(body.data.materials)-1
    for i in indices:body.data.polygons[i].material_index=slot
    return material


def make_eyes(body,arm,role,material,parts):
    """Curved exposed eye caps follow a narrow eyelid opening, avoiding protruding spheres."""
    bv=BVHTree.FromPolygons([v.co for v in body.data.vertices],[list(p.vertices) for p in body.data.polygons])
    for side,centre in enumerate(eye_centres(role)):
        hit,_,_,_=bv.ray_cast(Vector((centre.x,-1,centre.z)),Vector((0,1,0)),2)
        vertices=[(centre.x,(hit.y if hit else centre.y)-.0027,centre.z)]
        uvs=[((side+.5)/2,(ROLES.index(role)+.5)/4)];faces=[];segments=32
        for ring in range(1,7):
            r=ring/6
            for i in range(segments):
                a=i*math.tau/segments
                dx=.0152*r*math.cos(a);dz=.0056*r*math.sin(a)
                hit,_,_,_=bv.ray_cast(Vector((centre.x+dx,-1,centre.z+dz)),Vector((0,1,0)),2)
                y=(hit.y if hit else centre.y)-.0005-.0022*(1-r*r)
                vertices.append((centre.x+dx,y,centre.z+dz))
                uvs.append(((side+.5+dx/.033)/2,(ROLES.index(role)+.5+dz/.033)/4))
                j=1+(ring-1)*segments+i;following=1+(ring-1)*segments+(i+1)%segments
                faces.append((0,j,following) if ring==1 else (j,following,following-segments,j-segments))
        mesh=bpy.data.meshes.new('Eye cap');mesh.from_pydata(vertices,[],faces)
        eye=bpy.data.objects.new('Eye_'+role,mesh);bpy.context.collection.objects.link(eye)
        uv=mesh.uv_layers.new(name='UVMap').data
        for loop in mesh.loops:uv[loop.index].uv=uvs[loop.vertex_index]
        mesh.materials.append(material)
        for p in eye.data.polygons:p.use_smooth=True
        group=eye.vertex_groups.new(name='head');group.add(list(range(len(eye.data.vertices))),1,'REPLACE')
        parts.append(eye)


def eyes_material(root):
    size=256;data=np.ones((size*4,size*2,4),dtype=np.float32)
    atlas=_read(root/'Assets/Characters/Roles/FaceSource.png');height,width=atlas.shape[0]//2,atlas.shape[1]//2
    # The eye surfaces use the same photographic texture coordinates as the face,
    # so the whites, eyelid edges and iris blend rather than making white stickers.
    y,x=np.mgrid[:size,:size];dx=((x+.5)/size-.5)*.033;dz=((y+.5)/size-.5)*.033
    for index,role in enumerate(ROLES):
        panel=atlas[(1-index//2)*height:(2-index//2)*height,(index%2)*width:(index%2+1)*width]
        for side,centre in enumerate(eye_centres(role)):
            points=np.column_stack((centre.x+dx.ravel(),np.full(size*size,centre.y),centre.z+dz.ravel()))
            uv=project(points,role)
            sx=np.clip((uv[:,0]*(width-1)).astype(int),0,width-1)
            sy=np.clip(((1-uv[:,1])*(height-1)).astype(int),0,height-1)
            data[index*size:(index+1)*size,side*size:(side+1)*size]=panel[sy,sx].reshape((size,size,4))
    image=_write('T_Role_Eyes',root/'Assets/Characters/Roles/T_Role_Eyes.png',data)
    material=bpy.data.materials.new('Role_Eyes');material.use_nodes=True
    nodes=material.node_tree.nodes;shader=nodes.get('Principled BSDF');sample=nodes.new('ShaderNodeTexImage');sample.image=image
    material.node_tree.links.new(sample.outputs['Color'],shader.inputs['Base Color'])
    shader.inputs['Roughness'].default_value=.26;shader.inputs['Specular IOR Level'].default_value=.30
    return material
