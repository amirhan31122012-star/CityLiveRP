"""Simple mobile sky geometry; no SkyAtmosphere or Lumen dependency."""
import unreal


def build_mobile_sky_material(root):
    path = root + '/M_CityMobileSky_v11'
    material = unreal.load_asset(path)
    if not material:
        material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            'M_CityMobileSky_v11', root, unreal.Material, unreal.MaterialFactoryNew())
    if not material:
        raise RuntimeError('Cannot create mobile sky material')
    material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('is_sky', True)
    edit = unreal.MaterialEditingLibrary
    # Rebuild only our generated sky, including a partially saved failed attempt.
    edit.delete_all_material_expressions(material)
    horizon = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    horizon.set_editor_property('constant', unreal.LinearColor(.42, .6, .8, 1))
    zenith = edit.create_material_expression(material, unreal.MaterialExpressionConstant3Vector)
    zenith.set_editor_property('constant', unreal.LinearColor(.08, .25, .55, 1))
    normal = edit.create_material_expression(material, unreal.MaterialExpressionVertexNormalWS)
    mask = edit.create_material_expression(material, unreal.MaterialExpressionComponentMask)
    mask.set_editor_property('r', False)
    mask.set_editor_property('g', False)
    mask.set_editor_property('b', True)
    absolute = edit.create_material_expression(material, unreal.MaterialExpressionAbs)
    lerp = edit.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate)
    for source, target, requested in ((normal, mask, None), (mask, absolute, None),
                                      (horizon, lerp, 'A'), (zenith, lerp, 'B'),
                                      (absolute, lerp, 'Alpha')):
        names = list(edit.get_material_expression_input_names(target))
        # Unary expression inputs can be unnamed. Passing the literal "Input"
        # then fails even though the C++ member is called Input.
        if requested is None and len(names) == 1:
            pin = names[0]
        elif requested in names:
            pin = requested
        else:
            raise RuntimeError('Sky input not found: ' + str(requested) + ' available=' + str(names))
        if not edit.connect_material_expressions(source, '', target, pin):
            raise RuntimeError('Cannot connect sky node ' + target.get_class().get_name() +
                               ' pin=' + repr(pin) + ' available=' + str(names))
    if not edit.connect_material_property(lerp, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR):
        raise RuntimeError('Cannot connect mobile sky output')
    edit.recompile_material(material)
    if not unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False):
        raise RuntimeError('Cannot save mobile sky material')
    return material


def install_mobile_sky(root, actors):
    material = build_mobile_sky_material(root)
    for actor in list(actors.get_all_level_actors()):
        if isinstance(actor, unreal.SkyAtmosphere) or actor.get_actor_label() == 'CityLive_MobileSky':
            actors.destroy_actor(actor)
    sphere = unreal.load_asset('/Engine/BasicShapes/Sphere.Sphere')
    if not sphere:
        raise RuntimeError('Engine sky sphere missing')
    sky = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0))
    sky.set_actor_label('CityLive_MobileSky')
    sky.set_actor_scale3d(unreal.Vector(2000, 2000, 2000))  # Radius ~1km.
    component = sky.static_mesh_component
    component.set_static_mesh(sphere)
    component.set_material(0, material)
    component.set_collision_profile_name('NoCollision')
    component.set_editor_property('cast_shadow', False)
    return sky
