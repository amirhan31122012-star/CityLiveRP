"""Portable mesh naming and package validation for the authored city."""
from pathlib import Path
import hashlib
import re


def collision_kind(name):
    if name.startswith(('Curb_weed','Sparse_grass','Fine_lawn_grass','Birch_')):
        return 'D'
    if name.startswith(('Road_', 'Police_parking')):
        return 'R'
    if name.startswith('District_ground'):
        return 'G'
    if name.startswith('Police_building'):
        return 'P'
    if name.startswith(('Apartment_building', 'Foundation', 'Local_shop', 'Entrance_portal',
                        'Entrance_step', 'Access_path', 'Sidewalk', 'Curb_', 'Walk_segment',
                        'Playground_surface', 'Sandbox_board', 'Sand', 'Bench_',
                        'Tree_trunk', 'Streetlight_pole', 'Canopy_column', 'Bollard')):
        return 'C'
    return 'D'


def imported_kind(name):
    match = re.search(r'(?:^|_)PD_([RGPCD])_', name)
    if not match:
        raise ValueError('Unknown district mesh: ' + name)
    return match[1]


def package_file(root, relative):
    root = Path(root).resolve()
    path = (root / relative).resolve()
    if not path.is_relative_to(root) or not path.is_file():
        raise ValueError('Missing or unsafe district file: ' + str(relative))
    return path


def validate_package(root, manifest):
    if manifest.get('version') != 1 or manifest.get('district') != 'PoliceDistrict':
        raise ValueError('Unsupported district package')
    for item in manifest['files']:
        path = package_file(root, item['file'])
        if hashlib.sha256(path.read_bytes()).hexdigest() != item['sha256']:
            raise ValueError('District checksum mismatch: ' + item['file'])
    package_file(root, manifest['fbx'])
    for material in manifest['materials']:
        for role in ('base','normal','packed','emissive'):
            if material.get(role):
                package_file(root, material[role]['file'])
    return True
