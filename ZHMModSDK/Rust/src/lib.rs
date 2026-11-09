use ::safer_ffi::prelude::*;
use quickentity_rs::convert_to_rt;
use quickentity_rs::qn_structs::Entity;
use rpkg_rs::WoaVersion;
use rpkg_rs::resource::partition_manager::PartitionManager;

// These helpers are called from `#[ffi_export]` functions, where a panic aborts the
// whole process. Every failure is returned as an error message instead.
fn read_as_entity(json_data: &[u8]) -> Result<Entity, String> {
    serde_path_to_error::deserialize(&mut serde_json::Deserializer::from_slice(json_data))
        .map_err(|e| format!("invalid QN entity JSON: {}", e))
}

fn to_c_json(json: serde_json::Result<String>) -> Result<char_p::Box, String> {
    let json = json.map_err(|e| format!("could not serialise RT JSON: {}", e))?;
    json.try_into().map_err(|_| "RT JSON contains a NUL byte".to_owned())
}

// Kept separate from the `#[ffi_export]` wrapper so the steps can use `?`.
fn convert_qn_entity_impl(json_data: &[u8]) -> Result<QnConvertedData, String> {
    // Try to parse the JSON data as a QN entity.
    let entity = read_as_entity(json_data)?;

    // Convert the QN entity to an RT entity.
    let (factory, factory_meta, blueprint, blueprint_meta) =
        convert_to_rt(&entity).map_err(|e| format!("conversion to RT failed: {:#}", e))?;

    // Convert to JSON, and then to the C representation.
    Ok(QnConvertedData {
        factory_json: to_c_json(serde_json::to_string(&factory))?,
        factory_meta_json: to_c_json(serde_json::to_string(&factory_meta))?,
        blueprint_json: to_c_json(serde_json::to_string(&blueprint))?,
        blueprint_meta_json: to_c_json(serde_json::to_string(&blueprint_meta))?,
    })
}

#[derive_ReprC]
#[repr(C)]
pub struct QnConvertedData {
    pub factory_json: char_p::Box,
    pub factory_meta_json: char_p::Box,
    pub blueprint_json: char_p::Box,
    pub blueprint_meta_json: char_p::Box,
}

/// The result of `convert_qn_entity`: exactly one of `data` and `error` is non-null.
/// (A Rust `Result` has no C layout, so it crosses the FFI boundary as two nullable pointers.)
#[derive_ReprC]
#[repr(C)]
pub struct QnConvertResult {
    pub data: Option<repr_c::Box<QnConvertedData>>,
    pub error: Option<char_p::Box>,
}

#[derive_ReprC]
#[repr(C)]
pub struct ResourceChunkEntry {
    pub rid: u64,
    pub chunk_index: u32,
}

#[derive_ReprC]
#[repr(C)]
pub struct ResourceChunkMap {
    pub entries: repr_c::Vec<ResourceChunkEntry>,
}

/// Converts a QuickEntity JSON entity to RT JSON and meta. Never returns null: on failure,
/// `data` is null and `error` holds the reason. Free the result with `free_qn_convert_result`.
#[ffi_export]
pub fn convert_qn_entity(qn_json: char_p::Ref<'_>) -> repr_c::Box<QnConvertResult> {
    let result = match convert_qn_entity_impl(qn_json.to_bytes()) {
        Ok(data) => QnConvertResult {
            data: Some(Box::new(data).into()),
            error: None,
        },
        Err(message) => QnConvertResult {
            data: None,
            // The message can only fail to convert if it contains a NUL byte; drop those.
            error: Some(message.replace('\0', "").try_into().unwrap_or_else(|_| c!("unknown error").to_owned())),
        },
    };

    Box::new(result).into()
}

#[ffi_export]
pub fn free_qn_convert_result(result: Option<repr_c::Box<QnConvertResult>>) {
    drop(result);
}

#[ffi_export]
pub fn get_resource_chunk_map(game_path: char_p::Ref<'_>) -> Option<repr_c::Box<ResourceChunkMap>> {
    let game_path = game_path.to_string();

    let mut entries = Vec::new();

    let partition_manager =
        match PartitionManager::from_game(game_path.into(), WoaVersion::HM3, true) {
            Ok(x) => x,
            Err(e) => {
                println!("Failed to load partition manager: {}", e);
                return None;
            }
        };

    for partition in partition_manager.partitions {
        for (info, _) in partition.latest_resources() {
            entries.push(ResourceChunkEntry {
                rid: (*info.rrid()).into(),
                chunk_index: partition.partition_info().id.index.try_into().ok()?,
            });
        }
    }

    Some(
        Box::new(ResourceChunkMap {
            entries: entries.into(),
        })
        .into(),
    )
}

#[ffi_export]
pub fn free_resource_chunk_map(map: Option<repr_c::Box<ResourceChunkMap>>) {
    drop(map);
}

#[::safer_ffi::cfg_headers]
#[test]
fn generate_headers() -> ::std::io::Result<()> {
    ::safer_ffi::headers::builder()
        .to_file("zhmmodsdk_rs.h")?
        .generate()
}
