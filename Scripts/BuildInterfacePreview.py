"""Offline visual review only. The game uses native Slate, not this HTML."""
import json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
data=json.loads((root/'Assets/UI/DistrictMap.json').read_text())
routes=[json.loads(line) for line in (root/'Assets/UI/RoutePreview.jsonl').read_text().splitlines()]
assert len(routes)==len(data['places'])
html=(root/'Scripts/InterfacePreviewTemplate.html').read_text()
html=html.replace('__MAPDATA__',json.dumps(data,ensure_ascii=False)).replace('__ROUTES__',json.dumps(routes))
(root/'Assets/UI/InterfacePreview.html').write_text(html,encoding='utf-8')
print('INTERFACE_PREVIEW_READY')
