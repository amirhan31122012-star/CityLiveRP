"""Run in Unreal Editor only. Exports locally licensed mesh; never uploads it."""
from datetime import datetime
from pathlib import Path
import os
import unreal

asset_path = "/Game/VehicleVarietyVol2/Skeletons/SK_Sedan_01a.SK_Sedan_01a"
sedan_asset = unreal.load_asset(asset_path)
if not isinstance(sedan_asset, unreal.SkeletalMesh):
    raise RuntimeError("Sedan mesh not found. Migrate VehicleVarietyVol2 into this project first.")

output_directory = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_saved_dir())) / "VehicleExport"
output_directory.mkdir(parents=True, exist_ok=True)
output_path = output_directory / ("SedanForDoors_" + datetime.now().strftime("%Y%m%d_%H%M%S_%f") + ".fbx")

task = unreal.AssetExportTask()
task.object = sedan_asset
task.filename = str(output_path)
task.automated = True
task.prompt = False
task.replace_identical = False
task.exporter = unreal.SkeletalMeshExporterFBX()
task.options = unreal.FbxExportOption()
task.options.set_editor_property("level_of_detail", False)
task.options.set_editor_property("export_morph_targets", False)
task.options.set_editor_property("export_preview_mesh", False)
task.options.set_editor_property("collision", False)
if not unreal.Exporter.run_asset_export_task(task) or not output_path.is_file() or output_path.stat().st_size < 64:
    raise RuntimeError("FBX export failed: " + str(task.errors))
token = os.environ.get("CITYLIVE_EXPORT_TOKEN", "")
if token and all(character.isdigit() or character == "_" for character in token):
    (output_directory / ("Export_" + token + ".ok")).write_text(str(output_path), encoding="utf-8")
unreal.log("CityLive: sedan FBX exported to " + str(output_path))
