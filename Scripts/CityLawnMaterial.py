"""Textured lawn shading for Blender previews, without added geometry."""
import bpy


def realistic_lawn(material):
    nodes, links = material.node_tree.nodes, material.node_tree.links
    for node in list(nodes):
        if node.name.startswith('CityLawn_'):
            nodes.remove(node)
    shader = nodes.get('Principled BSDF')
    source = next(n for n in nodes if n.type == 'TEX_IMAGE' and n.image and
                  'grass_ground_diff' in n.image.name)

    def node(kind, name):
        result = nodes.new(kind)
        result.name = 'CityLawn_'+name
        return result

    # Keep the photographic blade/soil detail but move its dry base into greens.
    tint = node('ShaderNodeMixRGB','GreenTint')
    tint.blend_type = 'MULTIPLY'
    tint.inputs[0].default_value = .85
    tint.inputs[2].default_value = (.62,1.65,.55,1)
    links.new(source.outputs['Color'],tint.inputs[1])
    position = node('ShaderNodeNewGeometry','Position')
    noise = node('ShaderNodeTexNoise','PatchNoise')
    noise.inputs['Scale'].default_value = .36
    noise.inputs['Detail'].default_value = 3
    links.new(position.outputs['Position'],noise.inputs['Vector'])
    ramp = node('ShaderNodeValToRGB','PatchBrightness')
    ramp.color_ramp.elements[0].position = .23
    ramp.color_ramp.elements[0].color = (.72,.78,.67,1)
    ramp.color_ramp.elements[1].position = .78
    ramp.color_ramp.elements[1].color = (1.12,1.12,1.0,1)
    links.new(noise.outputs['Fac'],ramp.inputs['Fac'])
    variation = node('ShaderNodeMixRGB','NaturalVariation')
    variation.blend_type = 'MULTIPLY'
    variation.inputs[0].default_value = 1
    links.new(tint.outputs['Color'],variation.inputs[1])
    links.new(ramp.outputs['Color'],variation.inputs[2])
    wear = node('ShaderNodeValToRGB','SparseDrySpots')
    wear.color_ramp.elements[0].position = .62
    wear.color_ramp.elements[0].color = (0,0,0,1)
    wear.color_ramp.elements[1].position = .77
    wear.color_ramp.elements[1].color = (.32,.32,.32,1)
    links.new(noise.outputs['Fac'],wear.inputs['Fac'])
    dry = node('ShaderNodeMixRGB','DryMix')
    dry.inputs[2].default_value = (.17,.13,.065,1)
    links.new(wear.outputs['Color'],dry.inputs[0])
    links.new(variation.outputs['Color'],dry.inputs[1])
    links.new(dry.outputs['Color'],shader.inputs['Base Color'])
    shader.inputs['Roughness'].default_value = .95


def grass_blade_variation(obj):
    material = bpy.data.materials.get('LawnBlades')
    if not material:
        material = obj.data.materials[0].copy()
        material.name = 'LawnBlades'
    obj.data.materials[0] = material
    nodes, links = material.node_tree.nodes, material.node_tree.links
    for node in list(nodes):
        if node.name.startswith('CityLawn_'):
            nodes.remove(node)
    noise = nodes.new('ShaderNodeTexNoise')
    noise.name = 'CityLawn_BladeNoise'
    noise.inputs['Scale'].default_value = 2
    geometry = nodes.new('ShaderNodeNewGeometry')
    geometry.name = 'CityLawn_BladePosition'
    ramp = nodes.new('ShaderNodeValToRGB')
    ramp.name = 'CityLawn_BladeShades'
    ramp.color_ramp.elements[0].color = (.035,.09,.012,1)
    ramp.color_ramp.elements[1].color = (.15,.21,.045,1)
    links.new(geometry.outputs['Position'],noise.inputs['Vector'])
    links.new(noise.outputs['Fac'],ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'],nodes.get('Principled BSDF').inputs['Base Color'])
