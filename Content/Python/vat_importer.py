"""
Python backend for the VAT Importer Editor Utility Widget.
"""

import unreal
import os
import json
import re

print("--- [VAT Importer] Attempting to load vat_importer.py ---")


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def _log(message):
    """log regular message in unreal"""
    print(f"[VAT_Importer.py] {message}")

def _log_error(message):
    """log error in unreal"""
    print(f"[VAT_Importer.py] ERROR: {message}")

# ============================================================================
# Utility Functions
# ============================================================================

def _initialize_task(fbx_file_path, ue_target_path):
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', fbx_file_path)
    task.set_editor_property('destination_path', ue_target_path)
    task.set_editor_property('replace_existing', True)  # re import
    task.set_editor_property('save', False)
    task.set_editor_property('automated', True)
    return task

def _parse_character_input(character_input):
    """
    parse input names, divided by comma
    """

    if not character_input or not character_input.strip():
        return []

    characters = [c.strip() for c in character_input.split(',')] #spaces
    characters = [c for c in characters if c] #empty

    _log(f"Parsed character input: {characters}")
    return characters

def _extract_character_name(filename):
    """
    remove prefix and suffix
    """
    name = os.path.splitext(filename)[0]
    name = re.sub(r'^(SM_|T_|MI_|M_)', '', name, flags=re.IGNORECASE)

    # exr/json:
    # T_rp_eric_Angry_pos -> rp_eric
    match = re.match(r'^(.+?)_([A-Z][a-z]+)_(pos|rot|data)$', name)
    if match:
        return match.group(1)

    return name

def _get_all_file(source_path, folder='geo', suffix='.fbx'):
    """
    get all fbx from geo/
    """
    path = os.path.join(source_path, folder)
    if not os.path.isdir(path):
        _log_error(f"Cannot find {folder}/ folder in {path}")
        return []
    try:
        all_files = [f for f in os.listdir(path) if f.lower().endswith( suffix )]
    except Exception as e:
        _log_error(f"Error reading {folder} folder: {e}")
        return []

    if not all_files:
        _log_error(f"Cannot find {suffix} file in {path}")
        return []

    return all_files

def _get_names_from_geo(source_path):
    """
    get all fbx from geo/ then extract names from them
    """

    all_fbx_files = _get_all_file(source_path, 'geo', '.fbx')

    characters = []
    for fbx in all_fbx_files:
        character_name = _extract_character_name(fbx)
        if character_name and character_name not in characters:
            characters.append(character_name)

    return characters

def _chars_to_process(source_path, character_name):
    # which names to import -----------------------------------------
    requested_characters = _parse_character_input(character_name)

    if requested_characters:
        _log(f"Using provided characters: {requested_characters}")
        target_characters = requested_characters
    else:
        _log(f"No character input.")
        target_characters = _get_names_from_geo(source_path)

    return target_characters

# ============================================================================
# -- 1. import fbx
# ============================================================================

def _build_fbx_import_task(fbx_file_path, ue_target_path):
    task = _initialize_task(fbx_file_path, ue_target_path)

    options = unreal.FbxImportUI()
    options.set_editor_property('import_as_skeletal', False) # import skeletal meshes - false
    options.set_editor_property('import_materials', False)
    options.set_editor_property('import_textures', False)

    # sm data
    sm_import_data = unreal.FbxStaticMeshImportData()
    options.set_editor_property('import_mesh', True)
    sm_import_data.set_editor_property('normal_import_method', unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
    sm_import_data.set_editor_property('build_nanite', False)
    sm_import_data.set_editor_property('remove_degenerates', False)
    sm_import_data.set_editor_property('auto_generate_collision', False)
    sm_import_data.set_editor_property('import_mesh_lo_ds', True)
    # allocate sm data to options
    options.set_editor_property('static_mesh_import_data', sm_import_data)

    task.set_editor_property('options', options)
    return task

def import_fbx(source_path, ue_target_path, character_name=""):
    """
    import fbx from geo
    if already existed, reimport
    """

    _log("=" * 60)
    _log("starting FBX import...")

    geo_path = os.path.join(source_path, 'geo')
    _log(f"geo path: {geo_path}")

    all_fbx_files = _get_all_file(source_path, 'geo', '.fbx')

    _log(f"Found {len(all_fbx_files)} FBX file(s) in total")

    requested_characters = _parse_character_input(character_name)

    # only search for given character sm
    if requested_characters:
        _log(f"Filtering for characters: {requested_characters}")

        fbx_files = []
        for fbx in all_fbx_files:
            character_name = _extract_character_name(fbx)
            if character_name in requested_characters:
                fbx_files.append(fbx)

        if not fbx_files:
            _log_error(f"Cannot find fbx file that matches '{character_name}' in {geo_path} ")
            return []
    else:
        _log(f"No character input, importing all fbx ")
        fbx_files = all_fbx_files

    # import tasks
    tasks = []
    imported_characters = []

    for fbx_name in fbx_files:
        full_fbx_path = os.path.join(geo_path, fbx_name)
        task = _build_fbx_import_task(full_fbx_path, ue_target_path)
        tasks.append(task)

        if fbx_name not in imported_characters:
            imported_characters.append(fbx_name)
        _log(f"---> import pending: {fbx_name}")

    # import execution
    try:
        asset_tools.import_asset_tasks(tasks)
        _log(f"finished importing {len(tasks)} tasks")
        _log(f"Imported characters: {imported_characters}")
        return imported_characters
    except Exception as e:
        _log_error(f"Error occurred during fbx import: {e}")
        return []

# ============================================================================
# -- 2. import exr
# ============================================================================

def _build_exr_import_task(fbx_file_path, ue_target_path):
    task = _initialize_task(fbx_file_path, ue_target_path)

    texture_factory = unreal.TextureFactory()
    texture_factory.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture_factory.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_16_BIT_DATA)
    texture_factory.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_HDR)

    task.set_editor_property('factory', texture_factory)
    return task

def import_exr(source_path, ue_target_path, character_name=""):
    """
    import exr from tex:
        -if character input: import only input characters' exrs
        -if character empty: find all chars from geo/ then import their exrs
    if already existed, reimport
    """

    _log("=" * 60)
    _log("Starting EXR texture import...")

    tex_path = os.path.join(source_path, 'tex')
    _log(f"tex path: {tex_path}")

    if not os.path.isdir(tex_path):
        _log_error(f"Cannot find tex/ folder in: {tex_path}")
        return False

    # Get all EXR files -----------------------------------------
    all_exr_files = _get_all_file(source_path, 'tex', '.exr')
    if len(all_exr_files) == 0:
        return False

    _log(f"Found {len(all_exr_files)} EXR file(s) in tex folder")

    # which names to import -----------------------------------------
    target_characters = _chars_to_process(source_path, character_name)
    if not target_characters:
        _log_error(f"Cannot find any character in geo folder")
        return False

    # filter exr by names -----------------------------------------
    exr_files = []
    filtered_characters = set() #exr has repeated names

    for exr in all_exr_files:
        character_name = _extract_character_name(exr)

        if character_name in target_characters:
            filtered_characters.add(character_name)
            exr_files.append(exr)

    if not exr_files:
        _log_error(f"No EXR files found for characters: {target_characters}")
        return False

    # log found names and exr -----------------------------------------
    for char in filtered_characters:
        exrs = [e for e in exr_files if _extract_character_name(e) == char]
        _log(f"{char}: {len(exrs)} exr files found")

    # import task
    tasks = []

    for fbx_name in exr_files:
        full_exr_path = os.path.join(tex_path, fbx_name)
        task = _build_exr_import_task(full_exr_path, ue_target_path)
        tasks.append(task)
        _log(f"---> import pending: {fbx_name}")

    # import execution
    try:
        asset_tools.import_asset_tasks(tasks)
        _log(f"finished importing {len(tasks)} tasks")
        return True
    except Exception as e:
        _log_error(f"Error occurred during fbx import: {e}")
        return False

# ============================================================================
# -- 3. create Mis
# ============================================================================

def _get_vat_data_map(data_path):
    """
    return {'rp_carla': ['Angry', 'Clap', ...], 'rp_eric': ['Angry', 'Clap', ...]}
    """
    data_map = {}
    if not os.path.isdir(data_path):
        _log_error(f"Cannot find data folder: {data_path}")
        return {}

    for f in os.listdir(data_path):
        if f.lower().endswith('_data.json'):
            parts = f.replace('.json', '').split('_')
            if len(parts) >= 2:
                anim = parts[-1]
                name = _extract_character_name(f)

            if name not in data_map:
                data_map[name] = []
            data_map[name].append(anim)

    # same anmi order
    for char in data_map:
        data_map[char].sort()

    _log(f"Data found: {data_map}")
    return data_map

def _find_texture_asset(ue_target_path, character_name, anim_name, suffix):
    """
    find tex assets in UE
    suffix: 'pos' / 'rot'
    """

    texture_name = f"T_{character_name}_{anim_name}_{suffix}"
    asset_path = f"{ue_target_path}/{texture_name}.{texture_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    _log_error(f"Cannot find texture uasset '{texture_name}'")
    return None

