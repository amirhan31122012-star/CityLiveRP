"""Regression of installer side effects using a model of UE's slot-preserving reimport.
This checks our Python installer; it is not an Unreal integration test.
"""
import json,runpy,shutil,sys,tempfile,types,unittest
from pathlib import Path
from unittest.mock import patch
ROOT=Path(__file__).resolve().parents[1]
LEGACY='/Game/CityLiveVehicles/Courier/SM_CityScooter'
CURRENT='/Game/CityLiveVehicles/Courier/SM_CityScooter_v2'

class Properties:
    def __init__(self,**values):self.values=values
    def set_editor_property(self,key,value):self.values[key]=value
    def get_editor_property(self,key):return self.values[key]

class Material:pass
class Mesh:
    def __init__(self,names,fallback=False):
        self.slots=[Properties(imported_material_slot_name='None' if fallback else n,material_slot_name=n) for n in names]
        self.assignments={}
    def get_editor_property(self,key):assert key=='static_materials';return self.slots
    def set_material(self,index,material):self.assignments[index]=material

class Installer:
    def __init__(self,project,names,existing=None,save_mesh=True,fallback=False):
        self.project=project;self.tasks=[];self.saved=[];self.logs=[]
        self.assets={LEGACY:Mesh(['paint','rubber','metal','seat','light','red'])}
        self.assets[LEGACY].assignments={0:'original material'}
        if existing is not None:self.assets[CURRENT]=Mesh(existing)
        def save(asset,**kwargs):
            if isinstance(asset,Mesh) and not save_mesh:return False
            self.saved.append(asset);return True
        def create(name,folder,*args):
            asset=Material();self.assets[folder+'/'+name]=asset;return asset
        def import_tasks(tasks):
            for task in tasks:
                self.tasks.append(task);path=task.destination_path+'/'+task.destination_name
                # Critical UE behaviour from Animation.log: a reimport retains old slots.
                if path not in self.assets:self.assets[path]=Mesh(names,fallback)
        tools=types.SimpleNamespace(create_asset=create,import_asset_tasks=import_tasks)
        edit=types.SimpleNamespace(delete_all_material_expressions=lambda *a:None,
            create_material_expression=lambda *a:Properties(),connect_material_property=lambda *a:True,recompile_material=lambda *a:None)
        self.unreal=types.SimpleNamespace(
            Paths=types.SimpleNamespace(project_dir=lambda:str(project),convert_relative_path_to_full=lambda p:p),
            AssetToolsHelpers=types.SimpleNamespace(get_asset_tools=lambda:tools),MaterialEditingLibrary=edit,
            EditorAssetLibrary=types.SimpleNamespace(save_loaded_asset=save),load_asset=self.assets.get,
            Material=Material,StaticMesh=Mesh,MaterialFactoryNew=lambda:None,FbxFactory=lambda:None,
            AssetImportTask=types.SimpleNamespace,FbxImportUI=lambda:types.SimpleNamespace(static_mesh_import_data=Properties()),
            FBXImportType=types.SimpleNamespace(FBXIT_STATIC_MESH=1),LinearColor=lambda *a:a,
            MaterialExpressionConstant3Vector=object,MaterialExpressionConstant=object,
            MaterialProperty=types.SimpleNamespace(MP_BASE_COLOR=1,MP_ROUGHNESS=2,MP_METALLIC=3,MP_EMISSIVE_COLOR=4),log=self.logs.append)
    def run(self):
        with patch.dict(sys.modules,{'unreal':self.unreal}):
            runpy.run_path(str(ROOT/'Scripts/ImportCourierScooter.py'))

class ScooterImportTest(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.project=Path(self.temp.name)
        source=self.project/'Assets/Vehicles/Courier';source.mkdir(parents=True)
        for name in ('SM_CityScooter.fbx','manifest.json'):shutil.copyfile(ROOT/'Assets/Vehicles/Courier'/name,source/name)
        (self.project/'Saved').mkdir();self.marker=self.project/'Saved/CourierScooter.json'
        self.marker.write_text('{"revision":2}')
        self.names=['Scooter_'+key for key in json.loads((source/'manifest.json').read_text())['materials']]
    def assert_legacy_untouched(self,env):
        self.assertEqual(env.assets[LEGACY].assignments,{0:'original material'})
        self.assertNotIn(env.assets[LEGACY],env.saved)
    def assert_colours(self,env):
        mesh=env.assets[CURRENT];self.assertEqual(len(mesh.assignments),10)
        for i,slot in enumerate(mesh.slots):
            key=slot.get_editor_property('material_slot_name').removeprefix('Scooter_')
            self.assertIs(mesh.assignments[i],env.assets['/Game/CityLiveVehicles/Courier/M_Scooter_'+key])
    def test_upgrade_keeps_legacy_light_slot_out_of_new_asset(self):
        env=Installer(self.project,list(reversed(self.names)));env.run()
        self.assert_legacy_untouched(env);self.assert_colours(env)
        self.assertFalse(env.tasks[0].save)
        self.assertEqual(Path(env.tasks[0].filename).name,'SM_CityScooter.fbx')
        marker=json.loads(self.marker.read_text());self.assertEqual(marker['asset_path'],CURRENT)
        self.assertEqual(marker['import_revision'],3)
        self.assertIn(env.assets[CURRENT],env.saved)
    def test_repeat_install_and_slot_name_fallback(self):
        env=Installer(self.project,self.names,fallback=True);env.run();env.run()
        self.assertEqual(len(env.tasks),2);self.assert_legacy_untouched(env);self.assert_colours(env)
    def test_unknown_slot_aborts_before_assignment_or_success_marker(self):
        env=Installer(self.project,self.names,existing=self.names+['light'])
        with self.assertRaisesRegex(RuntimeError,'Unknown scooter material slot light'):env.run()
        self.assertFalse(env.assets[CURRENT].assignments);self.assertFalse(self.marker.exists())
        self.assert_legacy_untouched(env)
    def test_missing_current_slot_does_not_publish_success(self):
        env=Installer(self.project,self.names,existing=self.names[:-1])
        with self.assertRaisesRegex(RuntimeError,'layout differs'):env.run()
        self.assertFalse(env.assets[CURRENT].assignments);self.assertFalse(self.marker.exists())
    def test_failed_mesh_save_invalidates_previous_success(self):
        env=Installer(self.project,self.names,save_mesh=False)
        with self.assertRaisesRegex(RuntimeError,'Cannot save scooter'):env.run()
        self.assertFalse(self.marker.exists());self.assert_legacy_untouched(env)

if __name__=='__main__':unittest.main()
