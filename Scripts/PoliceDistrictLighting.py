"""Ambient fill for readable walls/shadows; keep directional shadows enabled."""
import unreal

REVISION=1


def configure_sun(component):
    component.set_editor_property('intensity',10.0)
    component.set_editor_property('cast_shadows',True)
    component.set_editor_property('light_source_angle',3.0)


def configure_sky(component):
    component.set_editor_property('intensity',2.2)
    # Python retains the legacy name although the editor label says Solid Color.
    component.set_editor_property('lower_hemisphere_is_black',True)
    component.set_editor_property('lower_hemisphere_color',unreal.LinearColor(.18,.21,.25,1))
