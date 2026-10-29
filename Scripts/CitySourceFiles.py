"""Discover the selected Modern City export without changing source files."""
import json
from pathlib import Path
from urllib.parse import unquote, urlsplit

PARTS = ('Road', 'Sidewalk', 'Parking', 'Green Area', 'Building', 'Towers')

def local_uri(root, uri):
    if urlsplit(uri).scheme or uri.startswith(('//', '\\\\')):
        raise ValueError('External URI is not a local asset: ' + uri)
    path = (root / unquote(uri)).resolve()
    if not path.is_relative_to(root.resolve()):
        raise ValueError('Asset path escapes its folder: ' + uri)
    return path

def discover(root):
    root = Path(root)
    if not root.is_dir():
        raise ValueError('City folder not found: ' + str(root))
    result = []
    for part in PARTS:
        candidates = sorted(p for p in root.iterdir() if p.is_dir() and
                            p.name.casefold().startswith(part.casefold()) and
                            'gltf' in p.name.casefold())
        if len(candidates) != 1:
            raise ValueError('Expected one GLTF folder for ' + part + ': ' + str(candidates))
        files = sorted(candidates[0].glob('*.gltf')) + sorted(candidates[0].glob('*.glb'))
        if len(files) != 1:
            raise ValueError('Expected one scene in ' + str(candidates[0]))
        source = files[0]
        if source.suffix.lower() == '.gltf':
            scene = json.loads(source.read_text(encoding='utf-8-sig'))
            for resource in scene.get('buffers', []) + scene.get('images', []):
                uri = resource.get('uri', '')
                if uri and not uri.startswith('data:'):
                    path = local_uri(source.parent, uri)
                    if not path.is_file():
                        raise ValueError('Missing file: ' + str(path))
                    if 'byteLength' in resource and path.stat().st_size != resource['byteLength']:
                        raise ValueError('Incorrect buffer size: ' + str(path))
        result.append((part, source))
    return result
