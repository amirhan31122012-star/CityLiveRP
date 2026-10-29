"""Give materials stable IDs before Blender imports/renames duplicate names."""
import json
import os
from pathlib import Path
import re
import struct
from urllib.parse import quote
from CitySourceFiles import local_uri


def stage_scene(source, output, label):
    source, output = Path(source), Path(output)
    chunks = []
    if source.suffix.lower() == '.glb':
        raw = source.read_bytes()
        offset = 12
        while offset < len(raw):
            length, kind = struct.unpack_from('<II', raw, offset)
            chunks.append((kind, raw[offset+8:offset+8+length]))
            offset += 8+length
        if not chunks or chunks[0][0] != 0x4e4f534a:
            raise ValueError('GLB JSON chunk missing')
        scene = json.loads(chunks[0][1])
    else:
        scene = json.loads(source.read_text(encoding='utf-8-sig'))
    staged = output / (label + '_stable' + source.suffix.lower())
    for index, mat in enumerate(scene.get('materials', [])):
        mat['name'] = 'CL_' + label + '_' + str(index).zfill(4)
    # The copy lives in Saved, not beside user assets. Keep buffers and images
    # external, referring back to the untouched originals (no 1.5GB copy).
    for item in scene.get('buffers', []) + scene.get('images', []):
        uri = item.get('uri')
        if uri and not uri.startswith('data:'):
            file = local_uri(source.parent, uri)
            try:
                relative = os.path.relpath(file, staged.parent)
            except ValueError:
                relative = str(file)  # Windows absolute path when drives differ.
            item['uri'] = quote(relative.replace('\\', '/'), safe='/:')
    encoded = json.dumps(scene, ensure_ascii=False).encode('utf-8')
    if chunks:
        encoded += b' ' * (-len(encoded) % 4)
        chunks[0] = (0x4e4f534a, encoded)
        body = b''.join(struct.pack('<II', len(blob), kind) + blob for kind, blob in chunks)
        staged.write_bytes(struct.pack('<4sII', b'glTF', 2, 12+len(body)) + body)
    else:
        staged.write_bytes(encoded)
    return staged


def bind_materials(meshes, records, all_materials=None):
    """Unify Blender's .001 variants by source ID, never by the original name."""
    lookup = {record['slot']: record for record in records}
    candidates = list(all_materials) if all_materials is not None else [
        material for obj in meshes for material in obj.data.materials if material is not None]
    canonical = {material.name: material for material in candidates if material.name in lookup}
    used = {}
    for obj in meshes:
        for index, material in enumerate(obj.data.materials):
            if material is None:
                raise RuntimeError('Empty material slot on ' + obj.name)
            name = material.name
            slot = name if name in lookup else re.sub(r'\.\d+$', '', name)
            if slot not in lookup:
                raise RuntimeError('Unknown source material ID: ' + obj.name + ' / ' + name)
            used[slot] = lookup[slot]
            if slot not in canonical:
                material.name = slot
                if material.name != slot:
                    raise RuntimeError('Source material ID name collision: ' + slot)
                canonical[slot] = material
            obj.data.materials[index] = canonical[slot]
    return [used[slot] for slot in sorted(used)]
