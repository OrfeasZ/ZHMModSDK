use quickentity_rs::convert_to_rt;
use quickentity_rs::qn_structs::Entity;
use quickentity_rs::rpkg_structs::ResourceMeta;
use quickentity_rs::rt_structs::{RTBlueprint, RTFactory};
use std::ffi::{c_char, CString};

fn read_as_entity(json_data: &[u8]) -> Entity {
    serde_path_to_error::deserialize(&mut serde_json::Deserializer::from_slice(json_data))
        .expect("Failed to parse file")
}

#[repr(C)]
pub struct QnConvertedData {
    pub factory_json: *mut c_char,
    pub factory_json_len: usize,
    pub factory_meta_json: *mut c_char,
    pub factory_meta_json_len: usize,
    pub blueprint_json: *mut c_char,
    pub blueprint_json_len: usize,
    pub blueprint_meta_json: *mut c_char,
    pub blueprint_meta_json_len: usize,
}

#[no_mangle]
pub extern "C" fn convert_qn_entity(data: *const u8, len: usize) -> QnConvertedData {
    // Try to parse the JSON data as a QN entity.
    let json_data = unsafe { std::slice::from_raw_parts(data, len) };
    let entity = read_as_entity(json_data);

    // Convert the QN entity to an RT entity.
    let (factory, factory_meta, blueprint, blueprint_meta) = convert_to_rt(&entity);

    // Convert to JSON.
    let factory_json = serde_json::to_vec(&factory).unwrap();
    let factory_meta_json = serde_json::to_vec(&factory_meta).unwrap();
    let blueprint_json = serde_json::to_vec(&blueprint).unwrap();
    let blueprint_meta_json = serde_json::to_vec(&blueprint_meta).unwrap();

    let factory_json_len = factory_json.len();
    let factory_meta_json_len = factory_meta_json.len();
    let blueprint_json_len = blueprint_json.len();
    let blueprint_meta_json_len = blueprint_meta_json.len();

    let factory_json_str = CString::new(factory_json).unwrap();
    let factory_meta_json_str = CString::new(factory_meta_json).unwrap();
    let blueprint_json_str = CString::new(blueprint_json).unwrap();
    let blueprint_meta_json_str = CString::new(blueprint_meta_json).unwrap();

    QnConvertedData {
        factory_json: factory_json_str.into_raw(),
        factory_json_len,
        factory_meta_json: factory_meta_json_str.into_raw(),
        factory_meta_json_len,
        blueprint_json: blueprint_json_str.into_raw(),
        blueprint_json_len,
        blueprint_meta_json: blueprint_meta_json_str.into_raw(),
        blueprint_meta_json_len,
    }
}

#[no_mangle]
pub extern "C" fn free_qn_converted_data(data: QnConvertedData) {
    unsafe {
        if !data.factory_json.is_null() {
            drop(CString::from_raw(data.factory_json));
        }

        if !data.factory_meta_json.is_null() {
            drop(CString::from_raw(data.factory_meta_json));
        }

        if !data.blueprint_json.is_null() {
            drop(CString::from_raw(data.blueprint_json));
        }

        if !data.blueprint_meta_json.is_null() {
            drop(CString::from_raw(data.blueprint_meta_json));
        }
    }
}
