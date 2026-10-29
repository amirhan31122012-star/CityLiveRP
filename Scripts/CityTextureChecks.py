"""Decode file textures before export; an unloaded Blender image is not missing."""
from pathlib import Path
import bpy


def ensure_textures_readable(images):
    errors = []
    for image in images:
        if image.source != 'FILE':
            continue
        path = Path(bpy.path.abspath(image.filepath, library=image.library))
        if image.packed_file is None and not path.is_file():
            errors.append(str(path) + ': file not found')
            continue
        try:
            # glTF can create image datablocks without decoding pixels (background
            # Blender in particular). has_data=False alone is not an error.
            if not image.has_data and image.packed_file is None:
                image.reload()
            if len(image.pixels) == 0:
                raise ValueError('image could not be decoded (no pixels)')
            _ = image.pixels[0]  # Force lazy pixel decoding without copying a 4K image.
            width, height = image.size
            if width <= 0 or height <= 0:
                raise ValueError('image has invalid dimensions')
            print('CITYLIVE_TEXTURE_OK', str(path), width, height, flush=True)
        except Exception as exc:
            errors.append(str(path) + ': ' + str(exc))
    if errors:
        raise RuntimeError('Texture file errors:\n' + '\n'.join(errors))
