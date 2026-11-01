# Role character test meshes

These are actual rigged FBX meshes, not the generated concept illustration.
`ActualModels.png` shows a Blender render of these meshes. Their faces, garments
and detail are substantially simpler than the approved concept. The civilian
keeps the existing base face; the three staff variants have modest head-shape,
hair and accessory differences. They are not exact recreations of the reference
person or the illustrated faces. There is no facial or individual finger rig.

## Attribution

Derived from **Casual Beige Knit Look** by **restore50**:
https://sketchfab.com/3d-models/casual-beige-knit-look-e92cefa2b26e47dc953cfdd16dbc076b

Licensed under **Creative Commons Attribution 4.0 International**:
https://creativecommons.org/licenses/by/4.0/

Changes by CityLive RP: the earlier corrected 21-bone rig, reduced geometry,
normalized skin weights, repacked UVs, modified head shapes, clothing colour textures, surface
details, glasses, hair, bags, equipment and separate courier backpack. The source
photographed skin texture is retained and modified in the wardrobe textures.
Keep this attribution and license link when distributing these derivatives.
In-game attribution is available through `/credits`, listed in `/help`.
No assets were extracted from Black Russia.

## Contents and regeneration

- `SK_CityCivilian.fbx`: black and white outfit, crossbody bag.
- `SK_CityPolice.fbx`: navy uniform and belt equipment.
- `SK_CityMedic.fbx`: white/teal clothing, glasses and gray temples.
- `SK_CityCourier.fbx`: red/black clothing and ginger hair.
- `SK_CityCourierPack.fbx`: separately rigged delivery backpack.
- `T_Role_*.png`: 2048px UV colour textures; FBX texture paths are not required.
- `manifest.json`: actual exported triangle counts and material palette.
- `Validation.json`: Blender FBX reimport checks; not an Unreal test report.

Run `blender -b -P Scripts/CreateRoleCharacters.py` from the project. The generator
also saves editable `/tmp/CityRole_<role>.blend` files. Run
`blender -b -P Tests/ValidateRoleCharactersBlender.py` to check the exported files.
Each body has fewer than 40,000 triangles, the same 21 bind bones and up to four
normalized influences. Object scales remain 1:1:1. Added clothing uses the same
rig; this does not guarantee freedom from clipping in every animation.

See `Docs/CharacterRoles.md` for installation and rollback. UE 5.8 import,
compilation, Android performance and in-game deformation require Windows/device
verification; Blender checks cannot establish those results.
