# Interface review assets

`InterfacePreview.html` is an offline design prototype, not the game's rendering
engine. Open it after downloading the repository so its relative artwork paths
resolve. `Preview_*.png` are clearly marked captures of that prototype.

The production implementation is native Slate (`SCityHud`, `SCityMap`,
`SCityV2Menu`, `SCityNavigationMenu`, `SCityChatMenu`). The prototype is not embedded
in the APK. The game map uses the generated C++ district data, not an invented
map or a screenshot copied from another game.

`DistrictMap.json` comes from `Scripts/ExportDistrictMap.py`, using the exact
Blender source hash of the packaged city. The hospital footprint and entry
positions follow the existing runtime component. `RoutePreview.jsonl` is emitted
by `Tests/CityMapRouteTest.cpp --dump`; regenerate it before
`python Scripts/BuildInterfacePreview.py` after changing navigation.

The login background `../Branding/CityLiveLoginCity.png` was generated for this
project with OpenAI image generation. Map geometry is authored CityLive RP data.
The city render used behind the HUD mock-up is an actual Blender district render;
the overlaid UI preview remains a design mock-up, not proof of UE compilation.

See `Docs/OrangeInterface.md` for installation, implementation and test limits.
