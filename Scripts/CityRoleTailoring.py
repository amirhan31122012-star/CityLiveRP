"""Tailored garment geometry, accessories and footwear on the preserved role rig."""
import bpy,math
from mathutils import Vector

def add(role,body,arm,parts,mats,bvh,box,line,surface,patch,finish):
    def raised(x,z,front=True,depth=.006):
        p=Vector(surface(x,z,front));p.y+=-depth if front else depth;return tuple(p)
    def seam(name,points,mat='Dark',width=.0012):
        # A projection miss is not a stitch: never draw fallback points in space.
        clean=[]
        for p in points:
            nearest,_,_,distance=bvh.find_nearest(Vector(p))
            if nearest is None or distance>.025:return
            clean.append(p)
        return line(name,clean,width,mat)
    def pocket(side,z,w=.075,h=.082):
        x=side*.115
        points=[raised(x-w/2,z-h),raised(x+w/2,z-h),raised(x+w/2,z),raised(x-w/2,z)]
        patch('Tailored pocket',points,'Navy' if role=='Police' else 'White' if role=='Medic' else 'Dark')
        seam('Pocket topstitch',points+[points[0]],'Steel' if role=='Civilian' else 'Dark',.0008)
        seam('Pocket welt',[raised(x-w/2,z+.004),raised(x+w/2,z+.004)],'Dark',.002)
    # Stand collar, separate edge stitching and zip pull above existing scan folds.
    if role in ('Civilian','Courier'):
        collar_vertices=[];collar_faces=[];steps=28
        for z,rx,ry in [(1.535,.079,.078),(1.583,.073,.068)]:
            for j in range(steps+1):
                a=-math.pi/2+.22+j*(2*math.pi-.44)/steps
                collar_vertices.append((rx*math.cos(a),-.115+ry*math.sin(a),z))
        for j in range(steps):collar_faces.append((j,j+1,j+steps+2,j+steps+1))
        mesh=bpy.data.meshes.new('Stand collar');mesh.from_pydata(collar_vertices,[],collar_faces)
        collar=bpy.data.objects.new('Stand collar',mesh);bpy.context.collection.objects.link(collar);finish(collar,'Dark' if role=='Civilian' else 'Red','spine_03')
        for poly in collar.data.polygons:poly.use_smooth=True
        for side in (-1,1):
            pocket(side,1.13,.082,.105)
            seam('Diagonal pocket zip',[raised(side*.075,1.11),raised(side*.15,1.155)],'Steel',.0013)
            # Continuous sleeve-panel topstitching, projected onto existing garment.
            seam('Jacket shoulder stitching',[raised(side*x,1.49-(x-.18)*.22) for x in (.18,.23,.28,.33,.38)],'Dark' if role=='Civilian' else 'White',.001)
        box('Zip slider',raised(0,1.47),(.01,.009,.025),'Steel',None,.002)
        seam('Jacket hem',[raised(x,1.01) for x in (-.19,-.12,0,.12,.19)],'Dark',.003)
    if role=='Civilian':
        # Real bag gusset, lid, zipper, hardware and seams, replacing a flat cuboid.
        box('Bag front pocket',(.11,-.27,1.06),(.205,.022,.11),'Dark','spine_01',.014)
        seam('Bag zipper',[(.015,-.285,1.115),(.205,-.285,1.115)],'Steel',.0014)
        for x in (.005,.215):
            seam('Bag piping',[(x,-.278,1.0),(x,-.278,1.145)],'GrayHair',.001)
            box('Strap buckle',(x,-.255,1.177),(.028,.014,.031),'Steel','spine_01',.005)
        # Discreet original chest emblem, no copied brand or game logo.
        x=-.11;z=1.385
        patch('Chest shield',[raised(x-.017,z+.021),raised(x+.017,z+.021),raised(x+.014,z-.011),raised(x,z-.021),raised(x-.014,z-.011)],'White')
        seam('Shield monogram',[raised(x-.008,z-.005,depth=.008),raised(x,z+.011,depth=.008),raised(x+.008,z-.005,depth=.008)],'Dark',.0013)
    if role=='Police':
        for side in (-1,1):
            pts=[raised(side*.012,1.51),raised(side*.093,1.485),raised(side*.058,1.422)]
            patch('Pressed shirt collar',pts,'Navy');seam('Collar seam',pts+[pts[0]],'Steel',.00065)
            pocket(side,1.35,.076,.081)
            # Three short gold rank bars laid on the shoulder board.
            for j in range(3):box('Shoulder rank',(side*(.186+j*.013),-.025,1.593),(.004,.036,.003),'Gold',None,.001)
        patch('Uniform tie',[raised(-.012,1.455,depth=.012),raised(.012,1.455,depth=.012),raised(.018,1.31,depth=.012),raised(0,1.287,depth=.012),raised(-.018,1.31,depth=.012)],'Dark')
        box('Duty belt pouch',(-.18,-.035,1.01),(.06,.06,.105),'Dark','pelvis',.012)
        # Shaped peaked cap with stitched band and a curved visor. Rigged to head.
        bpy.ops.mesh.primitive_uv_sphere_add(segments=36,ring_count=12,location=(0,-.135,1.835));o=bpy.context.object;o.name='Police cap crown';o.scale=(.096,.145,.05)
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True);finish(o,'Navy','head')
        for poly in o.data.polygons:poly.use_smooth=True
        for z in (1.812,1.826):
            line('Cap band',[(.087*math.cos(a),-.135+.14*math.sin(a),z) for a in [i*math.tau/48 for i in range(49)]],.004,'Dark','head')
        verts=[]
        for row in (0,1):
            for i in range(25):
                a=-math.pi/2+i*math.pi/24
                verts.append((.087*math.sin(a),-.135-(.14+row*.055*math.cos(a))*math.cos(a),1.813-row*.012))
        mesh=bpy.data.meshes.new('Curved cap visor');mesh.from_pydata(verts,[],[(i,i+1,26+i,25+i) for i in range(24)])
        o=bpy.data.objects.new('Curved cap visor',mesh);bpy.context.collection.objects.link(o);finish(o,'Dark','head')
        patch('Cap badge',[(-.012,-.284,1.838),(.012,-.284,1.838),(.009,-.284,1.817),(0,-.284,1.811),(-.009,-.284,1.817)],'Gold','head')
    if role=='Medic':
        for side in (-1,1):
            pts=[raised(side*.02,1.51,depth=.015),raised(side*.10,1.47,depth=.015),raised(side*.068,1.34,depth=.015),raised(side*.027,1.41,depth=.015)]
            patch('Coat lapel',pts,'White');seam('Lapel edge',pts,'GrayHair',.0008)
            pocket(side,1.12,.072,.074)
        verts=[];faces=[];segments=40
        for z,rx,ry in [(1.115,.195,.148),(.99,.23,.17),(.805,.235,.172)]:
            for j in range(segments+1):
                a=-math.pi/2+.14+j*(2*math.pi-.28)/segments
                verts.append((rx*math.cos(a),-.012+ry*math.sin(a),z))
        for row in range(2):
            for j in range(segments):
                a=row*(segments+1)+j;faces.append((a,a+segments+1,a+segments+2,a+1))
        mesh=bpy.data.meshes.new('Open coat hem');mesh.from_pydata(verts,[],faces)
        coat=bpy.data.objects.new('Open coat hem',mesh);bpy.context.collection.objects.link(coat);finish(coat,'White')
        for poly in coat.data.polygons:poly.use_smooth=True
        # Stethoscope with real earpieces, rubber tube and metal diaphragm.
        line('Stethoscope tube',[raised(-.061,1.50,depth=.022),raised(-.09,1.42,depth=.022),raised(-.06,1.27,depth=.022),raised(0,1.235,depth=.022),raised(.06,1.29,depth=.022),raised(.065,1.45,depth=.022)],.004,'Dark')
        box('Stethoscope diaphragm',raised(.062,1.285,depth=.027),(.031,.009,.031),'Steel',None,.014)
    if role=='Courier':
        for side in (-1,1):
            # Reflective split bands and a raised shoulder reinforcement panel.
            seam('Reflective jacket band',[raised(side*x,1.32,depth=.008) for x in (.025,.08,.14,.19)],'White',.006)
            pocket(side,1.115,.078,.085)
        box('Courier chest badge',raised(-.11,1.405),(.05,.009,.044),'White',None,.005)
