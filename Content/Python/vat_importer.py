"""
Python backend for the VAT Importer Editor Utility Widget.
"""

import unreal
import os
import json
import re

from unreal import MaterialInstanceConstantFactoryNew

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
    match = re.match(r'^(.+?)_([A-Za-z]+[0-9]*)_(pos|rot|data)$', name)
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

def _build_exr_import_task(exr_file_path, ue_target_path):
    task = _initialize_task(exr_file_path, ue_target_path)

    texture_factory = unreal.TextureFactory()
    # These settings may not be applied during import
    # will enforce set them after import
    texture_factory.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture_factory.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_16_BIT_DATA)
    texture_factory.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_HDR)

    task.set_editor_property('factory', texture_factory)
    return task

def _apply_exr_settings_to_texture(texture_asset):
    """
    apply correct settings to imported tex
    HDR (RGBA16F, no sRGB)
    No Mipmaps
    16 Bit Data group
    sRGB disabled
    """

    try:
        texture_asset.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_HDR)
        texture_asset.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        texture_asset.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_16_BIT_DATA)
        texture_asset.set_editor_property('srgb', False)
        return True
    except Exception as e:
        _log_error(f"Failed to apply settings to exr: {e}")
        return False



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

    for exr_name in exr_files:
        full_exr_path = os.path.join(tex_path, exr_name)
        task = _build_exr_import_task(full_exr_path, ue_target_path)
        tasks.append(task)
        _log(f"---> import pending: {exr_name}")

    # import execution
    try:
        asset_tools.import_asset_tasks(tasks)
        _log(f"finished importing {len(tasks)} tasks")

        for exr_name in exr_files:
            texture_name = os.path.splitext(exr_name)[0]
            asset_path = f"{ue_target_path}/{texture_name}.{texture_name}"

            if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
                texture_asset = unreal.EditorAssetLibrary.load_asset(asset_path)
                if _apply_exr_settings_to_texture(texture_asset):
                    _log(f"Applied settings to: {exr_name}")
            else:
                _log_error(f"Could not find imported uasset: {exr_name} to apply settings")

        _log(f"Settings applied to imported textures")

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
            parts = f.replace('_data.json', '').split('_')
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

def _create_or_get_mi(ue_target_path, mi_name, parent_material):
    """
    create or get existing material instance
    """

    mi_path = f"{ue_target_path}/{mi_name}.{mi_name}"

    if unreal.EditorAssetLibrary.does_asset_exist(mi_path):
        _log(f"{mi_name} already exists, will update")
        return unreal.EditorAssetLibrary.load_asset(mi_path)

    try:
        mi_asset = asset_tools.create_asset(asset_name=mi_name,
                                            package_path=ue_target_path,
                                            asset_class=unreal.MaterialInstanceConstant,
                                            factory=unreal.MaterialInstanceConstantFactoryNew()
                                            )
        # set parent
        if mi_asset and parent_material:
            mi_asset.set_editor_property('parent', parent_material)
            # unreal.EditorAssetLibrary.save_loaded_asset(mi_asset)
            _log(f"new MI {mi_name} created")
            return mi_asset
        else:
            _log_error(f"Cannot create MI {mi_name}")
            return None

    except Exception as e:
        _log_error(f"Error occurred during create or get MI material: {e}")
        return None

def _set_MI_texture_parm(mi_asset, parm_name, tex_asset):
    """
    set texture parm for MI
    """
    if not tex_asset or not mi_asset:
        return False

    try:
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(mi_asset, parm_name, tex_asset)
        return True
    except Exception as e:
        _log_error(f"Error occurred during set texture parm {parm_name}: {e}")
        return False

def _set_MI_scalar_parm(mi_asset, parm_name, value):
    """
    set texture parm for MI
    """
    if not mi_asset:
        return False

    try:
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(mi_asset, parm_name, value)
        return True
    except Exception as e:
        _log_error(f"Error occurred during set scalar parm {parm_name}: {e}")
        return False

def _set_MI_static_switch_parm(mi_asset, parm_name, value):
    """
    set static_switch parm for MI
    """
    if not mi_asset:
        return False

    try:
        unreal.MaterialEditingLibrary.set_material_instance_static_switch_parameter_value(mi_asset, parm_name, value)
        return True
    except Exception as e:
        _log_error(f"Error occurred during set static_switch parm {parm_name}: {e}")
        return False

def _read_json_bounds(json_path):
    """
    read json file, get bounds, return dict
    """
    try:
        with open(json_path, 'r') as f:
            data = json.load(f)
            if isinstance(data, list) and len(data) > 0:
                return data[0]
    except Exception as e:
        _log_error(f"Error occurred during read json {json_path}: {e}")
        return None

def create_MIs(source_path, ue_target_path, character_name="", base_parent_path=""):
    """
    create or update MIs:
        -if character input: only input characters'
        -if character empty: find all chars from geo/ then theirs
    if already existed, update


    For each character:
    1. First MI uses base_parent_path as parent
    2. Subsequent MIs use the first MI as parent
    3. Set texture parameters (Pos Tex, Rot Tex)
    4. Set bounds parameters from JSON data
    """

    _log("=" * 60)
    _log("Starting Material Instance creation...")

    # get char-anim mapping from json
    data_path = os.path.join(source_path, 'data')
    data_map = _get_vat_data_map(data_path)

    if not data_map:
        _log_error("No animation data found in data folder")
        return False

    # which names to import -----------------------------------------
    target_characters = _chars_to_process(source_path, character_name)
    if not target_characters:
        _log_error(f"Cannot find any character in geo folder")
        return False

    # filter data_map by names -----------------------------------------
    filtered_data_map = {k: v for k, v in data_map.items() if k in target_characters}

    if not filtered_data_map:
        _log_error(f"No json data found for characters: {target_characters}")
        return False

    _log(f"Creating MIs for: {list(filtered_data_map.keys())}")

    base_parent_material = None
    if base_parent_path:
        if unreal.EditorAssetLibrary.does_asset_exist(base_parent_path):
            base_parent_material = unreal.EditorAssetLibrary.load_asset(base_parent_path)
        else:
            _log_error(f"Cannot find base_parent_path '{base_parent_path}'")
            return False

    first_mi = None

    # process each char
    for char_name, animations in filtered_data_map.items():
        _log(f"Processing character: {char_name} -- {animations}")

        for idx, anim_name in enumerate(animations, start=1):
            mi_name = f"MI_VAT_{char_name}_{anim_name}"

            # MI's parent
            if idx == 1:
                parent_mat = base_parent_material
            else:
                parent_mat = first_mi

            # create or get MI
            mi_asset = _create_or_get_mi(ue_target_path, mi_name, parent_mat)

            if not mi_asset:
                _log_error(f"Cannot create or get MI: {mi_name}")
                continue

            # store first MI
            if idx == 1:
                first_mi = mi_asset

            # setup textures
            pos_tex = _find_texture_asset(ue_target_path, char_name, anim_name, "pos")
            rot_tex = _find_texture_asset(ue_target_path, char_name, anim_name, "rot")

            if pos_tex:
                _set_MI_texture_parm(mi_asset, "Position Texture", pos_tex)
            if rot_tex:
                _set_MI_texture_parm(mi_asset, "Rotation Texture", rot_tex)

            # enable Legacy to each char's first MI
            if idx == 1:
                _set_MI_static_switch_parm(mi_asset, "Support Legacy Parameters and Instancing", True)

            # setup bounds from json
            json_filename = f"{char_name}_{anim_name}_data.json"
            json_path = os.path.join(data_path, json_filename)

            if os.path.exists(json_path):
                bound_data = _read_json_bounds(json_path)

                if bound_data:
                    _set_MI_scalar_parm(mi_asset, "Bound Max X", bound_data.get("Bound Max X"))
                    _set_MI_scalar_parm(mi_asset, "Bound Max Y", bound_data.get("Bound Max Y"))
                    _set_MI_scalar_parm(mi_asset, "Bound Max Z", bound_data.get("Bound Max Z"))
                    _set_MI_scalar_parm(mi_asset, "Bound Min X", bound_data.get("Bound Min X"))
                    _set_MI_scalar_parm(mi_asset, "Bound Min Y", bound_data.get("Bound Min Y"))
                    _set_MI_scalar_parm(mi_asset, "Bound Min Z", bound_data.get("Bound Min Z"))

                    _log(f"set bounds for {mi_name} from {json_filename}")

                else:
                    _log_error(f"Cannot read json data from {json_filename}")
            else:
                _log_error(f"JSON file not found: {json_path}")

            _log("=" * 60)
            _log(f"Material Instance creation for {char_name} complete.")

