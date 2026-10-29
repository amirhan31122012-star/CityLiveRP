"""Find an open, road-supported spawn instead of accepting the first floor hit."""
import math


def find_open_spawn(origin, extent, floor_at, corridor_clear):
    # Centred candidates first. Require a 5m-wide road footprint and 6m escape
    # corridors, tested at body and head heights by the engine adapter.
    grid = [(x, y) for x in range(1, 32) for y in range(1, 32)]
    grid.sort(key=lambda p: (p[0]-16)**2 + (p[1]-16)**2)
    support = [(x, y) for x in (-250, 0, 250) for y in (-250, 0, 250)]
    directions = [(math.cos(a*math.pi/4)*600, math.sin(a*math.pi/4)*600) for a in range(8)]
    for ix, iy in grid:
        x = origin[0] - extent[0] + 2*extent[0]*ix/32
        y = origin[1] - extent[1] + 2*extent[1]*iy/32
        z = floor_at(x, y)
        if z is None:
            continue
        floors = [floor_at(x+dx, y+dy) for dx, dy in support]
        if any(h is None or abs(h-z) > 8 for h in floors):
            continue
        if all(corridor_clear(x, y, z, dx, dy) for dx, dy in directions):
            return x, y, z
    raise RuntimeError('No open road spawn with 5m support and 6m escape corridors; map not activated')
