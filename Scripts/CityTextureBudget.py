"""Texture limits for the first Android city test; keep source images intact."""
CITY_TEXTURE_LIMITS = {'Color': 1024, 'Normal': 512, 'Data': 512}


def apply_city_texture_budget(texture, role):
    cap = CITY_TEXTURE_LIMITS[role]
    previous = int(texture.get_editor_property('max_texture_size'))
    # Zero means unlimited; never raise an existing stricter limit.
    limit = min(previous, cap) if previous > 0 else cap
    texture.set_editor_property('max_texture_size', limit)
    texture.set_editor_property('never_stream', False)
    return previous, limit
