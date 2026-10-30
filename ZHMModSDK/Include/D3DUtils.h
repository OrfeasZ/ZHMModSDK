#pragma once

#include <Windows.h>

#define REF_IID_PPV_ARGS(Val) IID_PPV_ARGS(&(Val).m_Ref)

// RAII wrapper around a COM pointer. Constructed without an AddRef so it
// composes with QueryInterface / IID_PPV_ARGS; reassignment AddRefs the new
// value and Releases the old one.
template<class T> struct ScopedD3DRef {
    using RefType = T*;

    ScopedD3DRef() = default;
    ScopedD3DRef(RefType p_Ref) : m_Ref(p_Ref) {}

    ScopedD3DRef(const ScopedD3DRef&) = delete;
    ScopedD3DRef& operator=(const ScopedD3DRef&) = delete;

    ScopedD3DRef(ScopedD3DRef&& p_Other) noexcept : m_Ref(p_Other.m_Ref) {
        p_Other.m_Ref = nullptr;
    }

    ScopedD3DRef& operator=(ScopedD3DRef&& p_Other) noexcept {
        if (this != &p_Other) {
            Reset();
            m_Ref = p_Other.m_Ref;
            p_Other.m_Ref = nullptr;
        }
        return *this;
    }

    ~ScopedD3DRef() {
        Reset();
    }

    ScopedD3DRef& operator=(RefType p_Ref) {
        Reset();
        m_Ref = p_Ref;
        if (m_Ref) {
            m_Ref->AddRef();
        }
        return *this;
    }

    void Reset() {
        if (m_Ref) {
            m_Ref->Release();
            m_Ref = nullptr;
        }
    }

    RefType operator->() const {
        return m_Ref;
    }

    operator RefType() const {
        return m_Ref;
    }

    explicit operator bool() const {
        return m_Ref != nullptr;
    }

    // Releases any held ref and returns the storage address for the caller
    // to write a new pointer into (e.g. via IID_PPV_ARGS).
    RefType* ReleaseAndGetPtr() {
        Reset();
        return &m_Ref;
    }

    RefType m_Ref = nullptr;
};

inline void BreakIfFailed(HRESULT p_Result) {
    if (FAILED(p_Result)) {
        DebugBreak();
    }
}

// Owning HANDLE wrapper that calls CloseHandle on destruction.
struct SafeHandle {
    SafeHandle() = default;

    SafeHandle(const SafeHandle&) = delete;
    SafeHandle& operator=(const SafeHandle&) = delete;

    SafeHandle& operator=(HANDLE p_Handle) {
        Reset();
        m_Handle = p_Handle;
        return *this;
    }

    ~SafeHandle() {
        Reset();
    }

    void Reset() {
        if (m_Handle) {
            CloseHandle(m_Handle);
            m_Handle = nullptr;
        }
    }

    explicit operator bool() const {
        return m_Handle != nullptr;
    }

    HANDLE m_Handle = nullptr;
};
