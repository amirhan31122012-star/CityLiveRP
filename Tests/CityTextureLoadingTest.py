"""Run: blender --background --python-exit-code 1 --python Tests/CityTextureLoadingTest.py"""
from pathlib import Path
import sys
import tempfile
import bpy

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Scripts'))
from CityTextureChecks import ensure_textures_readable

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'test texture.png'
    original = bpy.data.images.new('texture fixture', width=2, height=2)
    original.filepath_raw = str(path)
    original.file_format = 'PNG'
    original.save()
    bpy.data.images.remove(original)
    texture = bpy.data.images.load(str(path), check_existing=False)
    print('Texture lazy before validation:', not texture.has_data)
    ensure_textures_readable([texture])
    assert texture.has_data and tuple(texture.size) == (2, 2)
    bpy.data.images.remove(texture)

    # An unloaded datablock must not allow a missing file through.
    missing = bpy.data.images.new('missing fixture', width=1, height=1)
    missing.source = 'FILE'
    missing.filepath = str(Path(directory) / 'missing.png')
    try:
        ensure_textures_readable([missing])
    except RuntimeError as exc:
        assert 'file not found' in str(exc)
    else:
        raise AssertionError('Missing file was accepted')
    bpy.data.images.remove(missing)

    # Reject an existing corrupt image too, not just absent paths.
    path.write_bytes(b'not a PNG')
    corrupt = bpy.data.images.new('corrupt fixture', width=1, height=1)
    corrupt.source = 'FILE'
    corrupt.filepath = str(path)
    corrupt.reload()
    try:
        ensure_textures_readable([corrupt])
    except RuntimeError as exc:
        assert 'no pixels' in str(exc) or 'dimensions' in str(exc)
    else:
        raise AssertionError('Corrupt image was accepted')
    bpy.data.images.remove(corrupt)
print('City texture loading checks passed')
