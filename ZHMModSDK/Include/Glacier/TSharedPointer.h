#pragma once

template <typename T>
class TSharedPointer {
public:
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

    T* m_pObject;
};