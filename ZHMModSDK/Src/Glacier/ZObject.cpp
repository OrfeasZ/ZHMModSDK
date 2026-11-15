#include <Glacier/ZObject.h>

#include <nlohmann/json.hpp>

#include <ResourceLib_HM3.h>

#include "Functions.h"

STypeID* ZObjectRef::GetVoidType() {
    if (!Globals::TypeRegistry || !*Globals::TypeRegistry) {
        return nullptr;
    }

    static STypeID* s_VoidType = (*Globals::TypeRegistry)->GetTypeID("void");
    return s_VoidType;
}

bool ZDynamicObject::Set(const ZString& p_Key, const ZDynamicObject& p_Value) {
    auto s_Values = As<TArray<SDynamicObjectKeyValuePair>>();

    if (!s_Values) {
        return false;
    }

    for (auto& s_KeyValuePair : *s_Values) {
        if (s_KeyValuePair.sKey == p_Key) {
            s_KeyValuePair.value = p_Value;
            return true;
        }
    }

    s_Values->push_back({ p_Key, p_Value });

    return true;
}

bool ZDynamicObject::Set(size_t p_Index, const ZDynamicObject& p_Value) {
    auto s_Values = As<TArray<ZDynamicObject>>();

    if (!s_Values) {
        return false;
    }

    if (p_Index >= s_Values->size()) {
        return false;
    }

    s_Values->at(p_Index) = p_Value;

    return true;
}

bool ZDynamicObject::Get(const ZString& p_Key, ZDynamicObject& p_Value) const {
    auto s_Values = As<TArray<SDynamicObjectKeyValuePair>>();

    if (!s_Values) {
        return false;
    }

    for (auto& s_KeyValuePair : *s_Values) {
        if (s_KeyValuePair.sKey == p_Key) {
            p_Value = s_KeyValuePair.value;
            return true;
        }
    }

    return false;
}

bool ZDynamicObject::Get(size_t p_Index, ZDynamicObject& p_Value) const {
    auto s_Values = As<TArray<ZDynamicObject>>();

    if (!s_Values) {
        return false;
    }

    if (p_Index >= s_Values->size()) {
        return false;
    }

    p_Value = s_Values->at(p_Index);

    return true;
}

ZDynamicObject& ZDynamicObject::operator[](const ZString& p_Key) {
    auto s_Values = As<TArray<SDynamicObjectKeyValuePair>>();

    static ZDynamicObject s_NullObject = {};

    if (!s_Values) {
        s_NullObject = {};
        return s_NullObject;
    }

    for (auto& s_KeyValuePair : *s_Values) {
        if (s_KeyValuePair.sKey == p_Key) {
            return s_KeyValuePair.value;
        }
    }

    s_Values->push_back({ p_Key, {} });

    return s_Values->at(s_Values->size() - 1).value;
}

const ZDynamicObject& ZDynamicObject::operator[](const ZString& p_Key) const {
    auto s_Values = As<TArray<SDynamicObjectKeyValuePair>>();

    static ZDynamicObject s_NullObject = {};

    if (!s_Values) {
        s_NullObject = {};
        return s_NullObject;
    }

    for (auto& s_KeyValuePair : *s_Values) {
        if (s_KeyValuePair.sKey == p_Key) {
            return s_KeyValuePair.value;
        }
    }

    return s_NullObject;
}

const ZDynamicObject& ZDynamicObject::operator[](size_t p_Index) const {
    auto s_Values = As<TArray<ZDynamicObject>>();

    static ZDynamicObject s_NullObject = {};

    if (!s_Values) {
        s_NullObject = {};
        return s_NullObject;
    }

    if (p_Index >= s_Values->size()) {
        s_NullObject = {};
        return s_NullObject;
    }

    return s_Values->at(p_Index);
}

ZDynamicObject& ZDynamicObject::operator[](size_t p_Index) {
    auto s_Values = As<TArray<ZDynamicObject>>();

    static ZDynamicObject s_NullObject = {};

    if (!s_Values) {
        s_NullObject = {};
        return s_NullObject;
    }

    if (p_Index >= s_Values->size()) {
        s_NullObject = {};
        return s_NullObject;
    }

    return s_Values->at(p_Index);
}

ZDynamicObject* ZDynamicObject::begin() {
    auto s_Values = As<TArray<ZDynamicObject>>();

    if (!s_Values) {
        return nullptr;
    }

    return s_Values->begin();
}

ZDynamicObject* ZDynamicObject::end() {
    auto s_Values = As<TArray<ZDynamicObject>>();

    if (!s_Values) {
        return nullptr;
    }

    return s_Values->end();
}

bool ZDynamicObject::PushBack(const ZDynamicObject& p_Value) {
    auto s_Values = As<TArray<ZDynamicObject>>();

    if (!s_Values) {
        return false;
    }

    s_Values->push_back(p_Value);

    return true;
}

size_t ZDynamicObject::Size() const {
    if (auto s_Object = As<TArray<SDynamicObjectKeyValuePair>>()) {
        return s_Object->size();
    }

    if (auto s_Array = As<TArray<ZDynamicObject>>()) {
        return s_Array->size();
    }

    return 0;
}

TArray<ZString> ZDynamicObject::Keys() const {
    auto s_Values = As<TArray<SDynamicObjectKeyValuePair>>();

    if (!s_Values) {
        return {};
    }

    TArray<ZString> s_Keys;

    for (auto& s_KeyValuePair : *s_Values) {
        s_Keys.push_back(s_KeyValuePair.sKey);
    }

    return s_Keys;
}

void ZDynamicObject::Insert(size_t p_Index, const ZDynamicObject& p_Value) {
    auto s_Values = As<TArray<ZDynamicObject>>();

    if (!s_Values) {
        return;
    }

    s_Values->insert(p_Index, p_Value);
}

namespace {
    std::string ToStdString(const ZString& p_Value) {
        return std::string(p_Value.c_str(), p_Value.size());
    }

    nlohmann::json GameStructToJson(
        const IType* p_TypeInfo,
        const void* p_Data
    ) {
        if (!p_TypeInfo || !p_TypeInfo->pszTypeName || !p_Data) {
            return nullptr;
        }

        JsonString* s_Json = HM3_GameStructToJson(
            p_TypeInfo->pszTypeName,
            p_Data,
            p_TypeInfo->m_nTypeSize
        );

        if (!s_Json) {
            return nullptr;
        }

        const auto s_Result = nlohmann::json::parse(
            s_Json->JsonData,
            s_Json->JsonData + s_Json->StrSize,
            nullptr,
            false
        );

        HM3_FreeJsonString(s_Json);

        return s_Result.is_discarded() ? nlohmann::json(nullptr) : s_Result;
    }

    nlohmann::json ToJsonValue(const ZDynamicObject& p_Object) {
        if (p_Object.IsObject()) {
            nlohmann::json s_Result = nlohmann::json::object();

            for (const auto& s_Key : p_Object.Keys()) {
                ZDynamicObject s_Value;

                if (p_Object.Get(s_Key, s_Value)) {
                    s_Result[ToStdString(s_Key)] = ToJsonValue(s_Value);
                }
            }

            return s_Result;
        }

        if (p_Object.IsArray()) {
            nlohmann::json s_Result = nlohmann::json::array();

            for (size_t i = 0; i < p_Object.Size(); ++i) {
                s_Result.push_back(ToJsonValue(p_Object[i]));
            }

            return s_Result;
        }

        const auto* s_TypeID = p_Object.GetTypeID();

        if (!s_TypeID || s_TypeID->m_nTypeNum == 1 || p_Object.IsEmpty()) {
            return nullptr;
        }

        const auto* s_TypeInfo = s_TypeID->GetTypeInfo();

        if (!s_TypeInfo) {
            throw std::runtime_error(
                "Missing ZDynamicObject type information"
            );
        }

        if (const auto* s_Value = p_Object.As<ZString>()) {
            return ToStdString(*s_Value);
        }

        if (const auto* s_Value = p_Object.As<bool>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<int8_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<int16_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<int32_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<int64_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<uint8_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<uint16_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<uint32_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<uint64_t>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<float>()) {
            return *s_Value;
        }

        if (const auto* s_Value = p_Object.As<double>()) {
            return *s_Value;
        }

        if (const auto* s_Guid = p_Object.As<ZGuid>()) {
            return ToStdString(s_Guid->ToString());
        }

        if (const auto* s_RepositoryID = p_Object.As<ZRepositoryID>()) {
            return ToStdString(s_RepositoryID->ToString());
        }

        return GameStructToJson(
            s_TypeInfo,
            p_Object.GetData()
        );
    }
}

ZString ZDynamicObject::ToJson() const {
    const std::string s_Json = ToJsonValue(*this).dump();

    return ZString::AllocateFromCStr(s_Json.c_str());
}