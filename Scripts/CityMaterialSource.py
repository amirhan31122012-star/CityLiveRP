"""Preserve original glTF PBR inputs instead of relying on FBX shader translation."""
import base64
import json
from pathlib import Path
import struct
from CitySourceFiles import local_uri


def ensure_default_material(scene):
    """A glTF primitive may legally omit its material: make that default explicit."""
    primitives = [p for mesh in scene.get('meshes', []) for p in mesh.get('primitives', [])
                  if p.get('material') is None]
    if primitives:
        materials = scene.setdefault('materials', [])
        index = len(materials)
        # glTF default material uses the default PBR factors (white, roughness 1,
        # metallic 1). Do not borrow a neighbouring face's textured material.
        materials.append({'name': '_CityLiveDefault', 'pbrMetallicRoughness': {
            'baseColorFactor': [1, 1, 1, 1], 'roughnessFactor': 1, 'metallicFactor': 1}})
        for primitive in primitives:
            primitive['material'] = index
    return scene


def material_sources(source, output, part):
    source, output = Path(source), Path(output)
    binary = None
    if source.suffix.lower() == '.glb':
        raw = source.read_bytes()
        length, kind = struct.unpack_from('<II', raw, 12)
        if kind != 0x4e4f534a:
            raise ValueError('GLB JSON chunk missing')
        scene = json.loads(raw[20:20+length])
        offset = 20 + length
        if offset < len(raw):
            size, kind = struct.unpack_from('<II', raw, offset)
            if kind == 0x004e4942:
                binary = raw[offset+8:offset+8+size]
    else:
        scene = json.loads(source.read_text(encoding='utf-8-sig'))

    ensure_default_material(scene)

    def image_file(index):
        image = scene['images'][index]
        uri = image.get('uri')
        if uri and not uri.startswith('data:'):
            return str(local_uri(source.parent, uri))
        if uri:
            content = base64.b64decode(uri.split(',', 1)[1], validate=True)
            mime = uri.split(';', 1)[0]
        else:
            view = scene['bufferViews'][image['bufferView']]
            buffer = scene['buffers'][view.get('buffer', 0)]
            buf_uri = buffer.get('uri')
            if buf_uri and buf_uri.startswith('data:'):
                blob = base64.b64decode(buf_uri.split(',', 1)[1], validate=True)
            elif buf_uri:
                blob = local_uri(source.parent, buf_uri).read_bytes()
            else:
                blob = binary
            if blob is None:
                raise ValueError('Embedded texture buffer missing')
            start = view.get('byteOffset', 0)
            content = blob[start:start + view['byteLength']]
            mime = image.get('mimeType', 'image/png')
        folder = output / 'Textures'
        folder.mkdir(exist_ok=True)
        path = folder / (part + '_' + str(index) + ('.jpg' if 'jpeg' in mime else '.png'))
        path.write_bytes(content)
        return str(path)

    def texture(info):
        if not info:
            return None
        sampler = scene['textures'][info['index']]
        if 'source' not in sampler:
            raise ValueError('Unsupported compressed texture: ' + str(sampler))
        transform = info.get('extensions', {}).get('KHR_texture_transform', {})
        return {'file': image_file(sampler['source']),
                'uv': transform.get('texCoord', info.get('texCoord', 0)),
                'scale': transform.get('scale', [1, 1]), 'offset': transform.get('offset', [0, 0]),
                'rotation': transform.get('rotation', 0)}

    result = []
    for index, mat in enumerate(scene.get('materials', [])):
        pbr = mat.get('pbrMetallicRoughness', {})
        result.append({'slot': 'CL_' + part + '_' + str(index).zfill(4),
                       'original_name': mat.get('name', 'Material_' + str(index)),
                       'base': texture(pbr.get('baseColorTexture')),
                       'normal': texture(mat.get('normalTexture')),
                       'packed': texture(pbr.get('metallicRoughnessTexture')),
                       'emissive': texture(mat.get('emissiveTexture')),
                       'base_factor': pbr.get('baseColorFactor', [1, 1, 1, 1]),
                       'roughness': pbr.get('roughnessFactor', 1),
                       'metallic': pbr.get('metallicFactor', 1),
                       'emissive_factor': mat.get('emissiveFactor', [0, 0, 0]),
                       'alpha': mat.get('alphaMode', 'OPAQUE'),
                       'cutoff': mat.get('alphaCutoff', .5),
                       'two_sided': mat.get('doubleSided', False)})
    return result
