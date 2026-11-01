# Original courier scooter, revision 2

`SM_CityScooter.fbx` is the actual model used by the game; `ActualModel.png` and
`ActualSide.png` are Blender renders of that exported geometry, not concept art
or Unreal/device captures. This original CityLive project asset contains no
geometry extracted from another game.

The curved red/ivory panels, wheels, brakes, tread, suspension, controls, mirrors,
lights, vents, seat seams and power unit are authored by `Scripts/CreateCourierScooter.py`.
`manifest.json` records triangle count, dimensions, colours and material properties.
`Scripts/ImportCourierScooter.py` creates ten explicit UE PBR materials and assigns
every imported slot on each install. The import does not rely on automatic FBX
material translation. Reinstall through `InstallDistrictContent.cmd` after Pull.

The preserved 1.31 m wheelbase, saddle around 0.72 m, handlebar position, root
origin and scene units remain compatible with existing mounting/physics code.
The single static visual mesh does not animate wheel rotation or steering.
Gameplay uses its existing independent chassis/suspension collision.

Regenerate with Blender, then run `Tests/ValidateCourierScooterBlender.py` and
`Scripts/PreviewCourierScooter.py`. UE import, riding pose, night lighting and
Android FPS still require editor/device verification. Global renderer settings
were not changed.
