"""Run with Blender: validate that V2's derived marker positions hit real parking."""
from pathlib import Path
import bpy
from mathutils import Vector
from mathutils.bvhtree import BVHTree

root=Path(__file__).resolve().parents[1]
bpy.ops.wm.read_factory_settings(use_empty=True)
bpy.ops.import_scene.fbx(filepath=str(root/'Assets/Maps/PoliceDistrictGame/PoliceDistrict.fbx'))
vertices=[];faces=[];kinds=[];police=[]
for obj in bpy.context.scene.objects:
    if obj.type!='MESH' or not any('PD_'+k+'_' in obj.name for k in 'RGPC'):continue
    coords=[obj.matrix_world@v.co for v in obj.data.vertices]
    if 'PD_P_' in obj.name:police.extend(coords)
    offset=len(vertices);vertices.extend(coords)
    for poly in obj.data.polygons:
        faces.append([offset+i for i in poly.vertices]);kinds.append('R' if 'PD_R_' in obj.name else 'solid')
assert police,'Police landmark missing'
center=(Vector(tuple(min(v[i] for v in police) for i in range(3)))+Vector(tuple(max(v[i] for v in police) for i in range(3))))*.5
bvh=BVHTree.FromPolygons(vertices,faces)
markers=[]
for outward in (Vector((1,0,0)),Vector((0,1,0)),Vector((-1,0,0)),Vector((0,-1,0))):
    side=Vector((-outward.y,outward.x,0))
    for offset in (-16,0,16):
        point=center+outward*38+side*offset
        hit,normal,index,_=bvh.ray_cast(Vector((point.x,point.y,50)),Vector((0,0,-1)),55)
        if hit is None or kinds[index]!='R' or normal.z<.99:continue
        heights=[]
        for x in (-2.6,0,2.6):
            for y in (-1.35,0,1.35):
                p=hit+side*x+outward*y
                floor,n,_,_=bvh.ray_cast(p+Vector((0,0,2.5)),Vector((0,0,-1)),8.5)
                if floor is None or n.z<.995:break
                heights.append(floor.z)
        if len(heights)==9 and max(heights)-min(heights)<=.04:markers.append(tuple(hit))
    if markers:break
assert len(markers)==3,markers
# The authored Police_parking surface is centred (-78,24), 94 by 18 metres.
# Reject accidentally using a through road or a rooftop for the garage trigger.
for x,y,z in markers:
    assert -125<x<-31 and 15<y<33 and abs(z)<.2,(x,y,z)
print('PASS: three V2 markers lie on flat, real police parking:',markers)
