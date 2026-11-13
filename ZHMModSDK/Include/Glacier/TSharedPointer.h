#pragma once

#include <concepts>
#include <utility>

#include "ZSharedPointerTarget.h"

template <std::derived_from<ISharedPointerTarget> T>
class TSharedPointer {
public:
    TSharedPointer() = default;

    explicit TSharedPointer(T* p_pObject) : m_pObject(p_pObject) {
        if (m_pObject) {
            m_pObject->AddReference();
        }
    }

    TSharedPointer(const TSharedPointer& p_Other) : m_pObject(p_Other.m_pObject) {
        if (m_pObject) {
            m_pObject->AddReference();
        }
    }

    TSharedPointer(TSharedPointer&& p_Other) noexcept : m_pObject(p_Other.m_pObject) {
        p_Other.m_pObject = nullptr;
    }

    ~TSharedPointer() {
        if (m_pObject) {
            m_pObject->RemoveReference();
        }
    }

    TSharedPointer& operator=(TSharedPointer p_Other) noexcept {
        std::swap(m_pObject, p_Other.m_pObject);
        return *this;
    }

    T* operator->() const {
        return m_pObject;
    }

    T* operator->() {
        return m_pObject;
    }

    const T* GetTarget() const {
        return m_pObject;
    }

    T* GetTarget() {
        return m_pObject;
    }

    T* m_pObject = nullptr;
};