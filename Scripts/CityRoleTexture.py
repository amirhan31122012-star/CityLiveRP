"""Rasterize continuous clothing masks in the existing UVs, avoiding triangle-sized colour steps."""
import bpy
import numpy as np


def bake(body, role, palette, original, target, original_uv, size=2048):
    image = bpy.data.images.load(str(original), check_existing=True)
    pixels = np.empty(image.size[0] * image.size[1] * 4, dtype=np.float32)
    image.pixels.foreach_get(pixels)
    pixels = pixels.reshape((image.size[1], image.size[0], 4))
    yy, xx = np.mgrid[:size, :size]
    result = pixels[(yy * image.size[1] // size), (xx * image.size[0] // size)].copy()
    painted = np.zeros((size,size),dtype=bool)
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
        elif role == 'Courier':
            colour[np.abs(px) > .22] = colours['Dark']
        elif role == 'Medic':
            shirt = (np.abs(px) < .078) & (z > 1.015) & (z < 1.55) & (py < -.08)
            colour[shirt] = colours['Teal']
        trousers = z < .99
        colour[trousers] = colours['Teal' if role == 'Medic' else 'Navy' if role == 'Police' else 'Dark']
        shoes = z < .17
        colour[shoes] = colours['White' if role in ('Civilian','Medic') else 'Dark']
        # Subtle cloth grain, deterministic and independent of polygon boundaries.
        grain = 1 + .025*np.sin(x*1.7+y*2.9) + .015*np.sin(px*1800+z*1400)
        colour *= grain[:,None]
        skin = ((z > 1.56) & ((np.abs(px) < .075) | (z > 1.67))) | (hands > .5)
        source_uv = bary @ original_uv[list(tri.loops)]
        source_x = np.floor((source_uv[:,0] % 1) * image.size[0]).astype(int)
        source_y = np.floor((source_uv[:,1] % 1) * image.size[1]).astype(int)
        colour[skin] = pixels[source_y[skin], source_x[skin], :3]
        hair = skin & ((z > 1.825) | ((z > 1.79) & (np.abs(px) > .058) & (py > -.21)))
        colour[hair] = colours['Ginger' if role == 'Courier' else 'Hair']
        if role == 'Medic':
            colour[hair & (np.abs(px) > .05)] = colours['GrayHair']
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
            result[fill]=shifted[fill];painted[fill]=True
    output = bpy.data.images.new('T_Role_'+role, width=size, height=size, alpha=False)
    output.colorspace_settings.name = 'sRGB'
    output.pixels.foreach_set(result.ravel())
    output.filepath_raw = str(target)
    output.file_format = 'PNG'
    output.save()
    return output
