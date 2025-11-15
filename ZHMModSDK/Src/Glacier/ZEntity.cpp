#include "Glacier/ZEntity.h"

#include "Functions.h"
#include "Hooks.h"

void ZEntityRef::SetLogicalParent(ZEntityRef entityRef) {
    const auto s_Entity = GetEntity();
    ZEntityType* s_EntityType = Functions::ZEntityImpl_EnsureUniqueType->Call(s_Entity, 0);

    s_EntityType->m_nLogicalParentEntityOffset =
        reinterpret_cast<uintptr_t>(entityRef.m_pEntity) - reinterpret_cast<uintptr_t>(m_pEntity);
}

bool ZEntityRef::SetProperty(uint32_t p_PropertyId, const ZObjectRef& p_Value, bool p_InvokeChangeHandlers) {
    return Hooks::SetPropertyValue->Call(*this, p_PropertyId, p_Value, p_InvokeChangeHandlers);
}

void ZEntityRef::SignalInputPin(uint32_t p_PinId, const ZObjectRef& p_Data) const {
    Hooks::SignalInputPin->Call(*this, p_PinId, p_Data);
}

void ZEntityRef::SignalOutputPin(uint32_t p_PinId, const ZObjectRef& p_Data) const {
    Hooks::SignalOutputPin->Call(*this, p_PinId, p_Data);
}