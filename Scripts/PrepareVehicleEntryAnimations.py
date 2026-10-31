"""Run in UE editor after compilation. Generate actual AnimSequence tracks on the
character Blueprint's existing mannequin skeleton. No FBX retargeting or scaling.
"""
import json,os,sys
from pathlib import Path
import unreal
sys.path.insert(0,str(Path(__file__).parent))
from CityVehicleAnimationPose import sample,DURATION,REVISION,qinv,rotate

project=Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
folder=Path(os.environ['CITYLIVE_ENTRY_LOG'])
character=unreal.load_class(None,'/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C')
if not character:raise RuntimeError('Character Blueprint missing')
cdo=unreal.get_default_object(character)
component=cdo.get_editor_property('mesh')
mesh=component.get_editor_property('skeletal_mesh')
if not isinstance(mesh,unreal.SkeletalMesh):raise RuntimeError('Character Blueprint skeletal mesh missing')
skeleton=mesh.get_editor_property('skeleton')
reference={}
for bone in unreal.CityVehicleAnimationLibrary.get_reference_pose(mesh):
    name=str(bone.bone_name);parent=str(bone.parent_name)
    transform=bone.component_transform
    pos=transform.translation;rotation=transform.rotation;scale=transform.scale3d
    if any(abs(float(getattr(scale,a))-1)>1e-3 for a in ('x','y','z')):
        raise RuntimeError('Reference bone scale is not one: '+name)
    reference[name]=dict(pos=(pos.x,pos.y,pos.z),rot=(rotation.x,rotation.y,rotation.z,rotation.w),
                         parent=parent if parent not in ('None','') else None)
for bone in ('pelvis','head','spine_01','spine_02','spine_03','thigh_l','calf_l','foot_l','upperarm_l','lowerarm_l','hand_l'):
    if bone not in reference:raise RuntimeError('Required bone missing: '+bone)
height=reference['head']['pos'][2]-reference['pelvis']['pos'][2]
if not 30<height<120:raise RuntimeError('Invalid mannequin reference pose: head/pelvis height '+str(height))
rotation=component.get_relative_transform().rotation
inverse=qinv((rotation.x,rotation.y,rotation.z,rotation.w))
forward=rotate(inverse,(1,0,0));up=rotate(inverse,(0,0,1))
tools=unreal.AssetToolsHelpers.get_asset_tools()
root='/Game/CityLiveCharacters/Vehicle'
for clip in ('Enter','Exit','EnterRight','ExitRight','DriverIdle'):
    name='AS_City'+clip
    sequence=unreal.load_asset(root+'/'+name)
    if not sequence:
        factory=unreal.AnimSequenceFactory();factory.set_editor_property('target_skeleton',skeleton)
        sequence=tools.create_asset(name,root,unreal.AnimSequence,factory)
    if not isinstance(sequence,unreal.AnimSequence) or sequence.get_editor_property('skeleton')!=skeleton:
        raise RuntimeError('Cannot create compatible sequence '+name)
    controller=sequence.get_editor_property('controller')
    if not controller:raise RuntimeError('Animation data controller missing: '+name)
    frames=30 if clip=='DriverIdle' else round(DURATION*30)
    samples=[sample(reference,i/30,clip,forward,up) for i in range(frames+1)]
    controller.open_bracket('CityLive full-size vehicle animation',False)
    try:
        controller.remove_all_bone_tracks(False)
        controller.set_frame_rate(unreal.FrameRate(30,1),False)
        controller.set_number_of_frames(unreal.FrameNumber(frames),False)
        for bone in reference:
            if not controller.add_bone_curve(bone,False):raise RuntimeError('Cannot add bone '+bone)
            keys=[s[bone] for s in samples]
            if not controller.set_bone_track_keys(bone,
                [unreal.Vector(*key[0]) for key in keys],
                [unreal.Quat(*key[1]) for key in keys],
                [unreal.Vector(1,1,1) for key in keys],False):
                raise RuntimeError('Cannot write bone '+bone)
    finally:controller.close_bracket(False)
    sequence.set_editor_property('enable_root_motion',False)
    if not unreal.EditorAssetLibrary.save_loaded_asset(sequence,only_if_is_dirty=False):
        raise RuntimeError('Cannot save '+name)
    unreal.log('CITYLIVE_ENTRY_CLIP_READY '+sequence.get_path_name())
# Blueprints already use ABP_Unarmed's DefaultSlot. Runtime constructs UAnimMontage
# instances in that slot and samples them with the same clock as doors/actor/IK.
folder.mkdir(parents=True,exist_ok=True)
(folder/'complete.ok').write_text(str(REVISION),encoding='utf-8')
(project/'Saved/VehicleEntryAnimations.json').write_text(json.dumps(dict(revision=REVISION,
    skeleton=skeleton.get_path_name(),mesh=mesh.get_path_name(),duration=DURATION,scale=[1,1,1]),indent=2),encoding='utf-8')
unreal.log('CITYLIVE_FULL_SIZE_ENTRY_READY')
