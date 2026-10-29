"""glTF KHR_texture_transform: offset + rotation * scale * UV, about the origin."""
import math


def uv_rows(info):
    angle = info.get('rotation', 0)
    cosine, sine = math.cos(angle), math.sin(angle)
    sx, sy = info.get('scale', [1, 1])
    return (cosine*sx, -sine*sy), (sine*sx, cosine*sy)
