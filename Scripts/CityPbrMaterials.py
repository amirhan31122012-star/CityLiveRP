"""Build static city materials directly from glTF's original PBR texture paths."""
import hashlib
from pathlib import Path
import re
import unreal
from CityUvTransform import uv_rows


def key(name):
    return re.sub(r'[^a-z0-9]', '', str(name).lower())


def build_materials(records, destination):
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    edit = unreal.MaterialEditingLibrary
    prop = unreal.MaterialProperty
    textures = {}
    result = {}

    def sample(material, info, role):
        path = Path(info['file'])
        if not path.is_file():
            raise RuntimeError('Original city texture missing: ' + str(path))
        identity = (str(path), role)
        if identity not in textures:
            name = 'T_' + role + '_' + hashlib.sha256(str(path).encode()).hexdigest()[:20]
            task = unreal.AssetImportTask()
            task.filename = str(path)
            task.destination_path = destination + '/Textures'
            task.destination_name = name
            task.automated = True
            task.save = True
            tools.import_asset_tasks([task])
            tex = unreal.load_asset(task.destination_path + '/' + name)
            if not isinstance(tex, unreal.Texture2D):
                raise RuntimeError('Texture import failed: ' + str(path))
            tex.set_editor_property('srgb', role == 'Color')
            tex.set_editor_property('max_texture_size', 2048)
            tex.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_NORMALMAP
                                    if role == 'Normal' else unreal.TextureCompressionSettings.TC_DEFAULT)
            if role == 'Normal':
                tex.set_editor_property('flip_green_channel', True)
            if not unreal.EditorAssetLibrary.save_loaded_asset(tex, only_if_is_dirty=False):
                raise RuntimeError('Cannot save texture: ' + str(path))
            textures[identity] = tex
        node = edit.create_material_expression(material, unreal.MaterialExpressionTextureSample)
        node.set_editor_property('texture', textures[identity])
        node.set_editor_property('sampler_type', {'Color': unreal.MaterialSamplerType.SAMPLERTYPE_COLOR,
                                 'Normal': unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL,
                                 'Data': unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR}[role])
        uv = edit.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate)
        uv.set_editor_property('coordinate_index', info['uv'])
        # glTF rotates about (0, 0), not the texture centre. Build the
        # affine matrix explicitly, with scale before rotation and offset last.
        uv.set_editor_property('u_tiling', 1)
        uv.set_editor_property('v_tiling', 1)
        if info.get('rotation', 0):
            coordinates = []
            for row in uv_rows(info):
                vector = edit.create_material_expression(material, unreal.MaterialExpressionConstant2Vector)
                vector.set_editor_property('r', row[0])
                vector.set_editor_property('g', row[1])
                dot = edit.create_material_expression(material, unreal.MaterialExpressionDotProduct)
                if not edit.connect_material_expressions(uv, '', dot, 'A'):
                    raise RuntimeError('Cannot connect rotated texture coordinates')
                if not edit.connect_material_expressions(vector, '', dot, 'B'):
                    raise RuntimeError('Cannot connect texture rotation matrix')
                coordinates.append(dot)
            append = edit.create_material_expression(material, unreal.MaterialExpressionAppendVector)
            for scalar, pin in zip(coordinates, ('A', 'B')):
                if not edit.connect_material_expressions(scalar, '', append, pin):
                    raise RuntimeError('Cannot combine rotated texture coordinates')
            uv = append
        else:
            uv.set_editor_property('u_tiling', info['scale'][0])
            uv.set_editor_property('v_tiling', info['scale'][1])
        if info['offset'] != [0, 0]:
            add = edit.create_material_expression(material, unreal.MaterialExpressionAdd)
            offset = edit.create_material_expression(material, unreal.MaterialExpressionConstant2Vector)
            offset.set_editor_property('r', info['offset'][0])
            offset.set_editor_property('g', info['offset'][1])
            edit.connect_material_expressions(uv, '', add, 'A')
            edit.connect_material_expressions(offset, '', add, 'B')
            uv = add
        edit.connect_material_expressions(uv, '', node, 'UVs')
        return node

    def constant(material, value):
        if isinstance(value, (list, tuple)):
            node = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
            node.set_editor_property('constant', unreal.LinearColor(*value[:3], 1))
        else:
            node = edit.create_material_expression(material, unreal.MaterialExpressionConstant)
            node.set_editor_property('r', value)
        return node

    def connect_scaled(material, node, channel, factor, target):
        if node is None:
            node, channel = constant(material, factor), ''
        else:
            multiply = edit.create_material_expression(material, unreal.MaterialExpressionMultiply)
            edit.connect_material_expressions(node, channel, multiply, 'A')
            edit.connect_material_expressions(constant(material, factor), '', multiply, 'B')
            node, channel = multiply, ''
        if not edit.connect_material_property(node, channel, target):
            raise RuntimeError('City material connection failed: ' + str(target))

    for record in records:
        mat = tools.create_asset('M_' + record['slot'], destination + '/Materials',
                                  unreal.Material, unreal.MaterialFactoryNew())
        if not mat:
            raise RuntimeError('Cannot create city material ' + record['slot'])
        mat.set_editor_property('two_sided', record['two_sided'])
        base = sample(mat, record['base'], 'Color') if record['base'] else None
        connect_scaled(mat, base, 'RGB', record['base_factor'][:3], prop.MP_BASE_COLOR)
        if record['alpha'] != 'OPAQUE':
            # Alpha-tested leaves/paint avoid translucent ground and sorting holes.
            mat.set_editor_property('blend_mode', unreal.BlendMode.BLEND_MASKED)
            mat.set_editor_property('opacity_mask_clip_value', record['cutoff'])
            connect_scaled(mat, base, 'A', record['base_factor'][3], prop.MP_OPACITY_MASK)
        packed = sample(mat, record['packed'], 'Data') if record['packed'] else None
        connect_scaled(mat, packed, 'G', record['roughness'], prop.MP_ROUGHNESS)
        connect_scaled(mat, packed, 'B', record['metallic'], prop.MP_METALLIC)
        if record['normal']:
            normal = sample(mat, record['normal'], 'Normal')
            if not edit.connect_material_property(normal, 'RGB', prop.MP_NORMAL):
                raise RuntimeError('Cannot connect city normal map')
        emissive = sample(mat, record['emissive'], 'Color') if record['emissive'] else None
        connect_scaled(mat, emissive, 'RGB', record['emissive_factor'], prop.MP_EMISSIVE_COLOR)
        edit.recompile_material(mat)
        if not unreal.EditorAssetLibrary.save_loaded_asset(mat, only_if_is_dirty=False):
            raise RuntimeError('Cannot save city material')
        result[key(record['slot'])] = mat
    return result


def assign_materials(mesh, materials):
    # Edit detached struct copies so nested setters cannot notify the mesh.
    slots = [slot.copy() for slot in mesh.get_editor_property('static_materials')]
    assignments = []
    for slot in slots:
        name = str(slot.get_editor_property('imported_material_slot_name'))
        material = materials.get(key(name))
        if material is None:
            material = materials.get(key(slot.get_editor_property('material_slot_name')))
        if material is None:
            raise RuntimeError('Unmatched original material slot: ' + mesh.get_name() + ' / ' + name)
        assignments.append((slot, material))
    # SetMaterial per slot triggers PostEditChange/rebuild for every material.
    # Validate all slots first, then publish the full array in one operation.
    for slot, material in assignments:
        slot.set_editor_property('material_interface', material)
    mesh.set_editor_property('static_materials', slots)
    unreal.log('CITYLIVE_MATERIALS_ASSIGNED ' + mesh.get_name() + ' slots=' + str(len(slots)))
    if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
        raise RuntimeError('Cannot save assigned city materials')
