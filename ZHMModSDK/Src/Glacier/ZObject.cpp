#include <Glacier/ZObject.h>

#include "Functions.h"

STypeID* ZObjectRef::GetVoidType() {
    static STypeID* s_VoidType = (*Globals::TypeRegistry)->m_types.find("void")->second;
    return s_VoidType;
}

bool ZDynamicObject::Set(const ZString& p_Key, const ZDynamicObject& p_Value) {
    auto s_Values = As<TArray<SDynamicObjectKeyValuePair>>();

    if (!s_Values) {
        return false;
    }

    //Functions::ZDynamicObject_Set->Call(this, p_Key, p_Value);

    // TODO: Fixme. The pool freeing thing is broken.
    for (auto& s_KeyValuePair : *s_Values) {
        if (s_KeyValuePair.sKey == p_Key) {
            s_KeyValuePair.value = p_Value;
            return true;
        }
    }

    s_Values->push_back({p_Key, p_Value});

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

    s_Values->push_back({p_Key, {}});

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
