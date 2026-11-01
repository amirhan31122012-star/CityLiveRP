"""Rasterize continuous clothing masks in the existing UVs, avoiding triangle-sized colour steps."""
import bpy
import numpy as np


def bake(body, role, palette, original, target, original_uv, size=2048, skin_tone=None):
    image = bpy.data.images.load(str(original), check_existing=True)
    pixels = np.empty(image.size[0] * image.size[1] * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    pixels = pixels.reshape((image.size[1], image.size[0], 4))
    yy, xx = np.mgrid[:size, :size]
    result = pixels[(yy * image.size[1] // size), (xx * image.size[0] // size)].copy()
    painted = np.zeros((size,size),dtype=bool)
    normal_map=np.full((size,size,4),[.5,.5,1,1],dtype=np.float32)
    surface_map=np.full((size,size,4),[.72,.25,0,1],dtype=np.float32)
    colours = {}
    for name, colour in palette.items():
        linear = np.asarray(colour[:3], dtype=np.float32)
        colours[name] = np.where(linear <= .0031308, linear * 12.92, 1.055 * linear ** (1 / 2.4) - .055)
    vertices = np.asarray([v.co[:] for v in body.data.vertices], dtype=np.float32)
    hand_weights = np.asarray([sum(g.weight for g in v.groups if body.vertex_groups[g.group].name.startswith('hand_')) for v in body.data.vertices])
    mesh = body.data
    mesh.calc_loop_triangles()
    uv = mesh.uv_layers.active.data
    for tri in mesh.loop_triangles:
        points = np.asarray([uv[i].uv[:] for i in tri.loops]) * size - .5
        lo = np.maximum(0, np.floor(points.min(axis=0)).astype(int))
        hi = np.minimum(size - 1, np.ceil(points.max(axis=0)).astype(int))
        if np.any(hi < lo):
            continue
        y, x = np.mgrid[lo[1]:hi[1]+1, lo[0]:hi[0]+1]
        a, b, c = points
        denominator = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
        if abs(denominator) < 1e-8:
            continue
        u = ((b[1]-c[1])*(x-c[0])+(c[0]-b[0])*(y-c[1])) / denominator
        v = ((c[1]-a[1])*(x-c[0])+(a[0]-c[0])*(y-c[1])) / denominator
        inside = (u >= -.001) & (v >= -.001) & (u+v <= 1.001)
        if not inside.any():
            continue
        x, y, u, v = x[inside], y[inside], u[inside], v[inside]
        bary = np.column_stack((u, v, 1-u-v))
        p = bary @ vertices[list(tri.vertices)]
        hands = bary @ hand_weights[list(tri.vertices)]
        px, py, z = p.T
        colour = np.empty((len(x), 3), dtype=np.float32)
        default = {'Civilian':'Dark', 'Police':'Navy', 'Medic':'White', 'Courier':'Red'}[role]
        colour[:] = colours[default]
        if role == 'Civilian':
            colour[(np.abs(px) > .25) & (z > 1.13)] = colours['White']
            cuff=(np.abs(px)>.59)&(z>1.13)&(z<1.6)
            colour[cuff]=colours['Dark']
            # Narrow twin stripes, with a separate charcoal cuff and shoulder panel.
            stripe=(np.abs(px)>.33)&(np.abs(px)<.57)&(z>1.40)&(py<-.035)
            colour[stripe]=colours['Dark']
        elif role == 'Police':
            colour[z>.99]=np.asarray([.205,.265,.35])
            placket=(np.abs(px)<.012)&(py<-.08)&(z>1.04)&(z<1.50)
            colour[placket]*=.72
        elif role == 'Courier':
            colour[np.abs(px) > .22] = colours['Dark']
        elif role == 'Medic':
            shirt = (np.abs(px) < .078) & (z > 1.015) & (z < 1.55) & (py < -.08)
            colour[shirt] = colours['Teal']
        trousers = z < .99
        colour[trousers] = colours['Teal' if role == 'Medic' else 'Navy' if role == 'Police' else 'Dark']
        shoes = z < .17
        colour[shoes] = colours['White' if role in ('Civilian','Medic') else 'Dark']
        # Metre-space seams and woven grain remain continuous across UV islands.
        weave=np.sin(px*3600+py*800)*np.sin(z*3900)*.018
        grain=1+weave+.014*np.sin(px*1700+z*1300)
        colour*=grain[:,None]
        front=py<-.07
        waist=np.exp(-((z-.995)/.004)**2)
        trouser_seam=np.exp(-((np.abs(px)-.205)/.003)**2)*(z>.2)*(z<.94)
        shoulder=np.exp(-((np.abs(px)-.245)/.003)**2)*(z>1.2)
        colour*=1-.12*np.maximum.reduce([waist,trouser_seam,shoulder])[:,None]
        # Cloth stitching: small interrupted topstitches along panel boundaries.
        stitches=(np.sin(z*1700)>0)&(trouser_seam>.3)
        colour[stitches]*=1.25
        # Real soles and toe panels, instead of a uniformly white scanned boot.
        outsole=(z<.043)
        colour[outsole]=colours['Dark']*.82
        sole=(z>=.043)&(z<.072)
        colour[sole]=colours['White']*.72
        toe=(z>.073)&(z<.105)&(py<-.17)
        colour[toe]*=.75
        lace=(z>.092)&(z<.155)&(py<-.065)&(py>-.185)&(np.abs(np.abs(px)-.12)<.03)&(np.abs(np.sin((py+(np.abs(px)-.12)*.35)*125))<.18)
        colour[lace]=colours['GrayHair' if role in ('Civilian','Medic') else 'White']
        skin = ((z > 1.56) & ((np.abs(px) < .075) | (z > 1.67))) | (hands > .5)
        source_uv = bary @ original_uv[list(tri.loops)]
        source_x = np.floor((source_uv[:,0] % 1) * image.size[0]).astype(int)
        source_y = np.floor((source_uv[:,1] % 1) * image.size[1]).astype(int)
        colour[skin] = pixels[source_y[skin], source_x[skin], :3]
        if skin_tone is not None:
            lum=colour[skin].mean(axis=1)
            colour[skin]=skin_tone[None,:]*np.clip(lum[:,None]/.68,.72,1.12)
        hair = skin & ((z > 1.825) | ((z > 1.79) & (np.abs(px) > .058) & (py > -.21)))
        colour[hair] = colours['Ginger' if role == 'Courier' else 'Hair']
        if role == 'Medic':
            colour[hair & (np.abs(px) > .05)] = colours['GrayHair']
        rough=np.full(len(x),.76);spec=np.full(len(x),.24)
        rough[shoes]=.61;rough[outsole]=.88;rough[skin]=.72;spec[skin]=.26
        rough[hair]=.94;spec[hair]=.03
        rough+=weave*.9
        surface_map[y,x,:3]=np.column_stack((np.clip(rough,.2,1),spec,np.zeros(len(x))))
        # Small fabric normal detail in the mesh UV tangent basis; no extra geometry.
        co=vertices[list(tri.vertices)];duv=np.asarray([uv[i].uv[:] for i in tri.loops])
        edge1,edge2=co[1]-co[0],co[2]-co[0];d1,d2=duv[1]-duv[0],duv[2]-duv[0]
        det=d1[0]*d2[1]-d1[1]*d2[0]
        if abs(det)>1e-9:
            tangent=(edge1*d2[1]-edge2*d1[1])/det
            bitangent=(edge2*d1[0]-edge1*d2[0])/det
            tangent/=max(np.linalg.norm(tangent),1e-8)
            bitangent-=tangent*np.dot(bitangent,tangent);bitangent/=max(np.linalg.norm(bitangent),1e-8)
            grad=np.column_stack((.07*np.cos(px*3600+py*800)*np.sin(z*3900),.018*np.cos(px*3600+py*800)*np.sin(z*3900),.07*np.sin(px*3600+py*800)*np.cos(z*3900)))
            grad[skin]=0
            n=np.column_stack((-grad@tangent,-grad@bitangent,np.ones(len(x))))
            n/=np.linalg.norm(n,axis=1)[:,None]
            normal_map[y,x,:3]=n*.5+.5
        result[y,x,:3] = np.clip(colour,0,1)
        result[y,x,3] = 1
        painted[y,x] = True
    # Island padding prevents source-colour fringes when mobile mipmaps shrink.
    for _ in range(8):
        previous=painted.copy()
        for axis,offset in ((0,1),(0,-1),(1,1),(1,-1)):
            neighbour=np.roll(previous,offset,axis=axis)
            fill=neighbour & ~painted
            shifted=np.roll(result,offset,axis=axis)
            result[fill]=shifted[fill]
            normal_map[fill]=np.roll(normal_map,offset,axis=axis)[fill]
            surface_map[fill]=np.roll(surface_map,offset,axis=axis)[fill]
            painted[fill]=True
    output = bpy.data.images.new('T_Role_'+role, width=size, height=size, alpha=False)
    output.colorspace_settings.name = 'sRGB'
    output.pixels.foreach_set(result.ravel())
    output.filepath_raw = str(target)
    output.file_format = 'PNG'
    output.save()
    for suffix,values in [('Normal',normal_map),('Surface',surface_map)]:
        # 1K auxiliary maps keep Android texture memory bounded.
        values=values[::2,::2].copy();aux=bpy.data.images.new('T_Role'+suffix+'_'+role,width=size//2,height=size//2,alpha=False)
        aux.colorspace_settings.name='Non-Color';aux.pixels.foreach_set(values.ravel())
        aux.filepath_raw=str(target.parent/('T_Role'+suffix+'_'+role+'.png'));aux.file_format='PNG';aux.save()
    return output
