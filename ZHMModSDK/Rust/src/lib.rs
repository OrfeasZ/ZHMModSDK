use ::safer_ffi::prelude::*;
use quickentity_rs::convert_to_rt;
use quickentity_rs::qn_structs::Entity;
use rpkg_rs::WoaVersion;
use rpkg_rs::resource::partition_manager::PartitionManager;

fn read_as_entity(json_data: &[u8]) -> Entity {
    serde_path_to_error::deserialize(&mut serde_json::Deserializer::from_slice(json_data))
        .expect("Failed to parse file")
}

#[derive_ReprC]
#[repr(C)]
pub struct QnConvertedData {
    pub factory_json: char_p::Box,
    pub factory_meta_json: char_p::Box,
    pub blueprint_json: char_p::Box,
    pub blueprint_meta_json: char_p::Box,
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

#[ffi_export]
pub fn convert_qn_entity(qn_json: char_p::Ref<'_>) -> Option<repr_c::Box<QnConvertedData>> {
    // Try to parse the JSON data as a QN entity.
    let json_data = qn_json.to_bytes();
    let entity = read_as_entity(json_data);

    // Convert the QN entity to an RT entity.
    let (factory, factory_meta, blueprint, blueprint_meta) = convert_to_rt(&entity).ok()?;

    // Convert to JSON.
    let factory_json = serde_json::to_string(&factory).ok()?;
    let factory_meta_json = serde_json::to_string(&factory_meta).ok()?;
    let blueprint_json = serde_json::to_string(&blueprint).ok()?;
    let blueprint_meta_json = serde_json::to_string(&blueprint_meta).ok()?;

    // And then to the C representation.
    let factory_json_cstr: char_p::Box = factory_json.try_into().ok()?;
    let factory_meta_json_cstr: char_p::Box = factory_meta_json.try_into().ok()?;
    let blueprint_json_cstr: char_p::Box = blueprint_json.try_into().ok()?;
    let blueprint_meta_json_cstr: char_p::Box = blueprint_meta_json.try_into().ok()?;

    Some(
        Box::new(QnConvertedData {
            factory_json: factory_json_cstr,
            factory_meta_json: factory_meta_json_cstr,
            blueprint_json: blueprint_json_cstr,
            blueprint_meta_json: blueprint_meta_json_cstr,
        })
        .into(),
    )
}

#[ffi_export]
pub fn free_qn_converted_data(data: Option<repr_c::Box<QnConvertedData>>) {
    drop(data);
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
