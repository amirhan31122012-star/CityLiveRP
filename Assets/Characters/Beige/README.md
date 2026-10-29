# CityLive RP male character

Derived from **Casual Beige Knit Look** by **restore50**:
https://sketchfab.com/3d-models/casual-beige-knit-look-e92cefa2b26e47dc953cfdd16dbc076b

Licensed under **Creative Commons Attribution 4.0 International**:
https://creativecommons.org/licenses/by/4.0/

Changes by CityLive RP: welded coincident vertices, added a 21-bone skeleton,
normalized and limited weights to four influences, straightened the arm bind
pose, smoothed the sleeve, centered the head and reconstructed the damaged
side symmetrically from the cleaner side. UV texture transforms were baked
into coordinates and embedded images extracted. This is a modified character,
not the original scan. The face and body symmetry differ from the original.

Keep this credit and license link with distributions of the model, and include
the character's attribution in game credits when distributing the game.
The license file does not itself implement an in-game credits screen.

About 198k triangles: still needs LOD/performance work for Android. No individual
finger or facial rig. Blender pose checks do not establish Unreal compatibility;
UE compilation, retargeting and gameplay still require Windows verification.

Version 3 separates and caps the trouser legs, widens the stance, fits knee/ankle
joints to the surface and aligns the shoes. Limb weights below the crotch
exclude the opposite leg. UE imports recompute normals for the edited mesh.
