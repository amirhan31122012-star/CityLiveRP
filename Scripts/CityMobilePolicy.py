"""Names refer to static meshes from Modern City, never the playable Sedan pawn."""
import re


def is_decorative_vehicle(name):
    words = re.sub(r'[^a-z0-9]+', '_', name.lower()).strip('_')
    return re.search(r'(^|_)(cars?|vehicles?|bus|buses|trucks?|vans?)(_|$)', words) is not None


def draw_distance(name):
    text = name.lower()
    if any(word in text for word in ('tree', 'bush', 'grass', 'foliage')):
        return 8000
    if any(word in text for word in ('bench', 'chair', 'bin', 'lamp', 'light')):
        return 6500
    if any(word in text for word in ('building', 'tower')):
        return 18000
    return 0  # Roads, sidewalks and collision stay continuous.
