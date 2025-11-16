"""
Python backend for the VAT Importer Editor Utility Widget.
"""

import unreal
import os
import json

print("--- [VAT Importer] Attempting to load vat_importer.py ---")


asset_tools = unreal.AssetToolsHelpers.get_asset_tools()


def _log(message):
    """log regular message in unreal"""
    print(f"[VAT_Importer.py] {message}")

def _log_error(message):
    """log error in unreal"""
    print(f"[VAT_Importer.py] ERROR: {message}")


# -- 1. import fbx

def _build_fbx_import_task(fbx_file_path, ue_destination_path):
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', fbx_file_path)
    task.set_editor_property('destination_path', ue_destination_path)
    task.set_editor_property('replace_existing', True)  # re import
    task.set_editor_property('save', False)
    task.set_editor_property('automated', True)

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
    import fbx in geo
    if already existed, reimport
    """

    _log("starting FBX import...")

    geo_path = os.path.join(source_path, 'geo')
    _log(f"geo path: {geo_path}")

    if not os.path.isdir(geo_path):
        _log_error(f"Cannot find geo/ folder in {geo_path}")
        return

    fbx_files = [f for f in os.listdir(geo_path) if f.endswith('.fbx')]
    if not fbx_files:
        _log_error(f"Cannot find fbx file in {geo_path}")
        return

    # only search for given character sm
    if character_name:
        _log(f"Only search for {character_name}")

        fbx_files = [f for f in fbx_files if character_name in f]
        if not fbx_files:
            _log_error(f"Cannot find fbx file that matches '{character_name}' in {geo_path} ")
            return

    tasks = []
    for fbx_name in fbx_files:
        full_fbx_path = os.path.join(geo_path, fbx_name)
        task = _build_fbx_import_task(full_fbx_path, ue_target_path)
        tasks.append(task)
        _log(f"import task append: {fbx_name}")

    try:
        asset_tools.import_asset_tasks(tasks)
        _log(f"finished importing {len(tasks)} tasks")
    except Exception as e:
        _log_error(f"Error occurred during fbx import: {e}")

    