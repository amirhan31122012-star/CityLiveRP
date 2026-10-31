"""Original full-body vehicle clips. Pure Python FK, cm, quaternions XYZW.
Runtime IK adjusts contact points without stretching. No bone scale channels vary.
"""
import math
DURATION=6.8
REVISION=1

def blend(t,a,b):
    v=max(0,min(1,(t-a)/(b-a)));return v*v*(3-2*v)

def phases(t,exit=False):
    if exit:
        return dict(door=blend(t,.4,1.1)*(1-blend(t,5.65,6.4)),
                    hand=blend(t,0,.35)*(1-blend(t,1.1,1.4))+blend(t,5.15,5.6)*(1-blend(t,6.4,6.65)),
                    sit=1-blend(t,3.3,4.5),first=1-blend(t,1.45,2.2),second=1-blend(t,2.3,3.1),
                    shift=1-blend(t,.25,1.1),turn=1-blend(t,1.15,3.2),
                    lean=blend(t,1.1,1.4)*(1-blend(t,4.5,4.85)),wheel=1-blend(t,0,.25))
    return dict(door=blend(t,1,1.7)*(1-blend(t,5.65,6.4)),
                hand=blend(t,.5,.95)*(1-blend(t,1.7,1.95))+blend(t,5.15,5.6)*(1-blend(t,6.4,6.65)),
                sit=blend(t,2.05,3.1),first=blend(t,3.2,3.95),second=blend(t,4.05,4.8),
                shift=blend(t,6.4,6.8),turn=blend(t,3.2,5.25),
                lean=blend(t,1.75,2.05)*(1-blend(t,4.8,5.4)),wheel=blend(t,6.4,6.8))

def add(a,b):return tuple(x+y for x,y in zip(a,b))
def sub(a,b):return tuple(x-y for x,y in zip(a,b))
def mul(a,s):return tuple(x*s for x in a)
def dot(a,b):return sum(x*y for x,y in zip(a,b))
def cross(a,b):return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def length(a):return math.sqrt(dot(a,a))
def norm(a):return mul(a,1/max(length(a),1e-12))
def qmul(a,b):return add(add(mul(b[:3],a[3]),mul(a[:3],b[3])),cross(a[:3],b[:3]))+(a[3]*b[3]-dot(a[:3],b[:3]),)
def qinv(a):return (-a[0],-a[1],-a[2],a[3])
def rotate(q,v):return qmul(qmul(q,v+(0,)),qinv(q))[:3]
def axis(a,deg):return mul(norm(a),math.sin(math.radians(deg)/2))+(math.cos(math.radians(deg)/2),)
def between(a,b):
    a=norm(a);b=norm(b);d=dot(a,b)
    if d<-.999999:
        perpendicular=cross(a,(1,0,0))
        if length(perpendicular)<.01:perpendicular=cross(a,(0,1,0))
        return axis(perpendicular,180)
    return norm(cross(a,b)+(1+d,))

def sample(reference,t,clip,forward=(0,1,0),up=(0,0,1)):
    """reference: ordered name -> dict(pos,rot,parent); return local bone keys."""
    p=phases(DURATION if clip=='DriverIdle' else t,clip.startswith('Exit'))
    exiting=clip.startswith('Exit');side=1 if clip.endswith('Right') else -1
    if clip=='DriverIdle':p=phases(DURATION)
    pose={n:dict(v,pos=tuple(v['pos']),rot=tuple(v['rot'])) for n,v in reference.items()}
    right=norm(cross(up,forward))
    def branch(root,q):
        if root not in pose:return
        pivot=pose[root]['pos']
        for name,value in pose.items():
            parent=name
            while parent is not None and parent!=root:parent=reference[parent]['parent']
            if parent!=root:continue
            value['pos']=add(pivot,rotate(q,sub(value['pos'],pivot)))
            value['rot']=norm(qmul(q,value['rot']))
    def aim(root,end,direction):
        if root in pose and end in pose:branch(root,between(sub(pose[end]['pos'],pose[root]['pos']),direction))
    interior=(t<1.4 if exiting else t>4.8) and clip!='DriverIdle'
    reach=p['hand'] if interior else 0
    lean=15*p['sit']+27*p['lean']+18*reach+(20*p['hand'] if not interior else 0)
    for bone,weight in (('spine_01',.5),('spine_02',.3),('spine_03',.2)):
        branch(bone,axis(right,lean*weight))
        branch(bone,axis(forward,-side*20*reach*weight))
    for side,sign in (('l',-1),('r',1)):
        # Lower the bind-pose A arms, then bring elbows forward for sitting.
        rest=add(mul(up,-1),mul(right,sign*.08))
        seated=add(mul(forward,.7),mul(up,-.7))
        direction=norm(add(mul(rest,1-p['sit']),mul(seated,p['sit'])))
        aim('upperarm_'+side,'lowerarm_'+side,direction)
        aim('lowerarm_'+side,'hand_'+side,norm(add(mul(up,-.8),mul(forward,.6*p['sit']))))
        target=norm(add(mul(up,-(1-p['sit'])),mul(forward,p['sit'])))
        aim('thigh_'+side,'calf_'+side,target)
        aim('calf_'+side,'foot_'+side,mul(up,-1))
        # Small back/forward steps while following the external door handle.
        if not exiting and clip!='DriverIdle':
            for start,end,amplitude in ((.95,1.7,-20),(1.7,2.05,12)):
                if start<t<end:
                    step=(t-start)/(end-start)
                    branch('thigh_'+side,axis(right,amplitude*math.sin(step*math.tau)*math.sin(step*math.pi)**2*(1 if side=='l' else -1)))
        if exiting and 5.65<t<6.4:
            step=(t-5.65)/.75
            branch('thigh_'+side,axis(right,20*math.sin(step*math.tau)*math.sin(step*math.pi)**2*(1 if side=='l' else -1)))
        # One coordinated walking step after rising, before closing the door.
        if exiting and 4.5<t<5.15:
            phase=(t-4.5)/.65
            angle=18*math.sin(phase*math.tau)*math.sin(phase*math.pi)**2*(1 if side=='l' else -1)
            branch('thigh_'+side,axis(right,angle))
        foot='foot_'+side
        if foot in pose:branch(foot,qmul(reference[foot]['rot'],qinv(pose[foot]['rot'])))
    result={}
    for name,value in pose.items():
        parent=value['parent']
        if parent is None:pos=value['pos'];rot=value['rot']
        else:
            inverse=qinv(pose[parent]['rot'])
            pos=rotate(inverse,sub(value['pos'],pose[parent]['pos']))
            rot=qmul(inverse,value['rot'])
        # Rotation-only FK must preserve every bind-pose local translation.
        if parent is not None:
            expected=rotate(qinv(reference[parent]['rot']),sub(reference[name]['pos'],reference[parent]['pos']))
            if length(sub(pos,expected))>1e-3:raise ValueError('Bone length changed: '+name)
        result[name]=(pos,norm(rot),(1,1,1))
    return result
