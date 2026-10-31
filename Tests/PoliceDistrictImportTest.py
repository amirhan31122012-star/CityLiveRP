"""Import transaction and collision policy; Unreal API is substituted here."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Scripts'))
from PoliceDistrictPolicy import collision_kind, imported_kind, validate_package


class Props:
    def __init__(self):self.values={}
    def set_editor_property(self,key,value):self.values[key]=value


class Vec:
    def __init__(self,x,y,z):self.x,self.y,self.z=x,y,z


class Mesh:
    def __init__(self,kind):self.kind=kind;self.body=Props()
    def get_name(self):return 'PD_'+self.kind+'_xp0_yp0_CLPD_000'
    def get_editor_property(self,key):return self.body


class Component(Props):
    def set_static_mesh(self,mesh):self.mesh=mesh
    def set_collision_profile_name(self,name):self.profile=name
    def recapture_sky(self):pass


class Actor:
    def __init__(self):self.static_mesh_component=Component();self.light_component=Component()
    def set_actor_label(self,label):self.label=label
    def get_actor_bounds(self,unused):
        if self.static_mesh_component.mesh.kind=='P':return Vec(-7800,6200,485),Vec(2900,1400,465)
        return Vec(0,0,0),Vec(20500,16500,5)


class ImportTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
        self.project=Path(self.temp.name)
        for folder in ('Saved','Config','Assets/Maps/PoliceDistrictGame'):(self.project/folder).mkdir(parents=True)
        self.marker=self.project/'Saved/ModernCityMap.txt'
        self.mobile=self.project/'Saved/CityV11.json'
        self.ini=self.project/'Config/DefaultEngine.ini'
        self.old='/Game/CityLiveMaps/ModernCity/Run_1_2_3/CityLive_ModernCity'
        self.marker.write_text(self.old)
        self.mobile.write_text(json.dumps({'version':'1.1.2','map':self.old,'graphics_baseline':'v1','mobile_sky':True}))
        self.ini.write_text('GameDefaultMap='+self.old+'\nEditorStartupMap='+self.old+'\nr.Substrate=True\n')
        self.before=[p.read_text() for p in (self.marker,self.mobile,self.ini)]
        self.package=self.project/'Assets/Maps/PoliceDistrictGame'
        (self.package/'PoliceDistrict.fbx').write_bytes(b'fixture')
        manifest={'version':1,'district':'PoliceDistrict','fbx':'PoliceDistrict.fbx','mesh_count':5,'triangles':151502,
                  'materials':[],'source_blend_sha256':'fixture','files':[{'file':'PoliceDistrict.fbx','sha256':hashlib.sha256(b'fixture').hexdigest()}]}
        (self.package/'manifest.json').write_text(json.dumps(manifest))
        self.meshes=[Mesh(k) for k in 'RGPCD'];self.actors=[];self.save=True;self.imported=False;self.new_level=False
        def import_tasks(tasks):self.imported=True
        def new_level(path):self.new_level=True;return True
        def spawn(*args):actor=Actor();self.actors.append(actor);return actor
        def trace(world,start,*args):
            actor=next(a for a in self.actors if hasattr(a.static_mesh_component,'mesh') and a.static_mesh_component.mesh.kind=='R')
            values=[None]*10;values[5]=Vec(start.x,start.y,0);values[7]=Vec(0,0,1);values[9]=actor
            return types.SimpleNamespace(to_tuple=lambda:values)
        options=lambda:types.SimpleNamespace(static_mesh_import_data=Props())
        self.levels=types.SimpleNamespace(new_level=new_level,save_current_level=lambda:self.save)
        actor_api=types.SimpleNamespace(spawn_actor_from_class=spawn)
        world_api=types.SimpleNamespace(get_editor_world=lambda:types.SimpleNamespace(get_world_settings=lambda:Props()))
        self.api=types.SimpleNamespace(
            Paths=types.SimpleNamespace(project_dir=lambda:str(self.project),convert_relative_path_to_full=lambda p:p),
            EditorAssetLibrary=types.SimpleNamespace(does_asset_exist=lambda p:False,list_assets=lambda *a,**k:list(range(5)),save_loaded_asset=lambda *a,**k:True),
            AssetToolsHelpers=types.SimpleNamespace(get_asset_tools=lambda:types.SimpleNamespace(import_asset_tasks=import_tasks)),
            AssetImportTask=lambda:types.SimpleNamespace(),FbxFactory=lambda:None,FbxImportUI=options,
            FBXImportType=types.SimpleNamespace(FBXIT_STATIC_MESH=0),StaticMesh=Mesh,load_asset=lambda i:self.meshes[i],
            CollisionTraceFlag=types.SimpleNamespace(CTF_USE_COMPLEX_AS_SIMPLE=0),
            LevelEditorSubsystem='levels',EditorActorSubsystem='actors',UnrealEditorSubsystem='world',
            get_editor_subsystem=lambda key:{'levels':self.levels,'actors':actor_api,'world':world_api}[key],
            StaticMeshActor=Actor,PlayerStart='start',DirectionalLight='sun',SkyLight='sky',Vector=Vec,Rotator=lambda *a:a,
            ComponentMobility=types.SimpleNamespace(STATIC=0,MOVABLE=1),load_class=lambda *a:object(),
            SystemLibrary=types.SimpleNamespace(line_trace_single=trace,sphere_trace_single=lambda *a:None),
            TraceTypeQuery=types.SimpleNamespace(TRACE_TYPE_QUERY1=0),DrawDebugTrace=types.SimpleNamespace(NONE=0),log=lambda *a:None)
        pbr=types.SimpleNamespace(build_materials=lambda *a,**k:self.assertFalse(k['texture_budget']),assign_materials=lambda *a:None)
        sky=types.SimpleNamespace(install_mobile_sky=lambda *a:None)
        spec=importlib.util.spec_from_file_location('district_import_test',ROOT/'Scripts/ImportPoliceDistrict.py')
        self.module=importlib.util.module_from_spec(spec)
        with patch.dict(sys.modules,{'unreal':self.api,'CityPbrMaterials':pbr,'CityMobileSky':sky}):spec.loader.exec_module(self.module)

    def run_import(self):
        with patch.dict('os.environ',{'CITYLIVE_DISTRICT_TOKEN':'4_5_6'}):self.module.main()

    def unchanged(self):self.assertEqual([p.read_text() for p in (self.marker,self.mobile,self.ini)],self.before)

    def test_new_city_replaces_startup_only_after_saved(self):
        self.run_import();state=json.loads(self.mobile.read_text())
        self.assertEqual(state['source'],'PoliceDistrict')
        self.assertEqual(self.marker.read_text(),state['map'])
        self.assertNotEqual(self.marker.read_text(),self.old)
        self.assertIn('r.Substrate=True',self.ini.read_text())
        previous=json.loads((self.project/'Saved/PoliceDistrictPrevious.json').read_text())
        self.assertEqual(previous['map'],self.old)
        for actor in self.actors:
            component=actor.static_mesh_component
            if hasattr(component,'mesh'):
                self.assertEqual(component.profile,'NoCollision' if component.mesh.kind=='D' else 'BlockAll')
        self.assertTrue((self.project/'Saved/PoliceDistrictImport/4_5_6/complete.ok').exists())

    def test_failed_map_save_preserves_old_startup(self):
        self.save=False
        with self.assertRaisesRegex(RuntimeError,'Cannot save district map'):self.run_import()
        self.unchanged()
        self.assertFalse((self.project/'Saved/PoliceDistrictImport/4_5_6/complete.ok').exists())

    def test_corrupt_package_stops_before_unreal_import(self):
        (self.package/'PoliceDistrict.fbx').write_bytes(b'corrupt')
        with self.assertRaisesRegex(ValueError,'checksum'):self.run_import()
        self.assertFalse(self.imported);self.unchanged()

    def test_missing_collision_stops_before_new_level(self):
        self.meshes[0].body=None
        with self.assertRaisesRegex(RuntimeError,'Missing collision body'):self.run_import()
        self.assertFalse(self.new_level);self.unchanged()


class PolicyTest(unittest.TestCase):
    def test_foliage_and_markings_do_not_block_player(self):
        for name in ('Curb_weed.005','Fine_lawn_grass_1','Sparse_grass_tufts','Birch_leaves.3','Lane_dash.4','Window_glass'):
            self.assertEqual(collision_kind(name),'D',name)
        for name in ('Apartment_building','Tree_trunk.4','Curb_EW','Sandbox_board_1'):
            self.assertEqual(collision_kind(name),'C',name)
        self.assertEqual(collision_kind('Police_building'),'P')
        self.assertEqual(collision_kind('Road_EW'),'R')
        self.assertEqual(imported_kind('PoliceDistrict_PD_R_xm3_yp0_CLPD_002'),'R')
        with self.assertRaises(ValueError):imported_kind('UnexpectedMesh')


if __name__=='__main__':unittest.main()
