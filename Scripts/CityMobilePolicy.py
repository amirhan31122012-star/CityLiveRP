"""Names refer to static meshes from Modern City, never the playable Sedan pawn."""
import re


def is_decorative_vehicle(name):
    words = re.sub(r'[^a-z0-9]+', '_', name.lower()).strip('_')
    return re.search(r'(^|_)(cars?|vehicles?|bus|buses|trucks?|vans?)(_|$)', words) is not None
