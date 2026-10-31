"""Spatial material batches for the authored district; leave editable parts intact."""
import math
import bpy


def export_batched_glb(objects, destination, cell_size=32):
    bpy.context.view_layer.update()
    groups = {}
    for obj in objects:
        material = obj.material_slots[0].material
        key = (math.floor(obj.location.x/cell_size), math.floor(obj.location.y/cell_size), material.name)
        vertices, faces, _ = groups.setdefault(key, ([], [], material))
        offset = len(vertices)
        vertices.extend(tuple(obj.matrix_world @ vertex.co) for vertex in obj.data.vertices)
        faces.extend(tuple(index+offset for index in polygon.vertices) for polygon in obj.data.polygons)
    bpy.ops.object.select_all(action='DESELECT')
    temporary = []
    try:
        for (x,y,mat_name), (vertices,faces,material) in groups.items():
            mesh = bpy.data.meshes.new(f'Cell_{x}_{y}_{mat_name}')
            mesh.from_pydata(vertices, [], faces)
            mesh.materials.append(material)
            obj = bpy.data.objects.new(mesh.name, mesh)
            bpy.context.scene.collection.objects.link(obj)
            obj.select_set(True)
            temporary.append(obj)
        bpy.ops.export_scene.gltf(filepath=str(destination), export_format='GLB',
                                  use_selection=True, export_cameras=False, export_lights=False)
    finally:
        for obj in temporary:
            mesh = obj.data
            bpy.data.objects.remove(obj, do_unlink=True)
            bpy.data.meshes.remove(mesh)
    return {'exported_mesh_objects': len(groups), 'batch_cell_metres': cell_size}
