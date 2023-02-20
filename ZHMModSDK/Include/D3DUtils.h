#pragma once

#define REF_IID_PPV_ARGS(Val) IID_PPV_ARGS(&Val.Ref)

template <class T>
struct ScopedD3DRef
{
    typedef T* RefType;

    ScopedD3DRef(RefType p_Ref) : Ref(p_Ref) {}

    ScopedD3DRef() : Ref(nullptr) {}

    ScopedD3DRef(const ScopedD3DRef& p_Other)
    {
        Ref = p_Other.Ref;

        if (Ref)
            Ref->AddRef();

        return *this;
    }

    ScopedD3DRef(ScopedD3DRef&& p_Other) noexcept
    {
        Ref = p_Other.Ref;
        p_Other.Ref = nullptr;
    }

    ~ScopedD3DRef()
    {
        if (Ref)
            Ref->Release();
    }

    ScopedD3DRef& operator=(const ScopedD3DRef& p_Other)
    {
        Reset();
        Ref = p_Other.Ref;

        if (Ref)
            Ref->AddRef();

        return *this;
    }

    ScopedD3DRef& operator=(ScopedD3DRef&& p_Other) noexcept
    {
        Ref = p_Other.Ref;
        p_Other.Ref = nullptr;
        return *this;
    }

    ScopedD3DRef& operator=(RefType p_Ref)
    {
        Reset();
        Ref = p_Ref;

        if (Ref)
            Ref->AddRef();

        return *this;
    }

    void Reset()
    {
        if (Ref)
            Ref->Release();

        Ref = nullptr;
    }

    RefType operator->() const
    {
        return Ref;
    }

    operator RefType() const
    {
        return Ref;
    }

    operator bool() const
    {
        return Ref != nullptr;
    }

    void* VTable() const
    {
        return *reinterpret_cast<void**>(Ref);
    }

    RefType* ReleaseAndGetPtr()
    {
        Reset();
        return &Ref;
    }

    RefType Ref;
};

inline void BreakIfFailed(HRESULT p_Result)
{
    if (FAILED(p_Result))
        DebugBreak();
}

struct SafeHandle
{
    SafeHandle(HANDLE p_Handle) : Handle(p_Handle) {}

    SafeHandle() : Handle(nullptr) {}

    SafeHandle(const SafeHandle&) = delete;

    SafeHandle(SafeHandle&& p_Other) noexcept
    {
        Handle = p_Other.Handle;
        p_Other.Handle = nullptr;
    }

    ~SafeHandle()
    {
        if (Handle)
            CloseHandle(Handle);
    }

    SafeHandle& operator=(const SafeHandle&) = delete;

    SafeHandle& operator=(SafeHandle&& p_Other) noexcept
    {
        Handle = p_Other.Handle;
        p_Other.Handle = nullptr;
        return *this;
    }

    SafeHandle& operator=(HANDLE p_Handle)
    {
        Reset();
        Handle = p_Handle;
        return *this;
    }

    void Reset()
    {
        if (Handle)
            CloseHandle(Handle);

        Handle = nullptr;
    }

    operator bool() const
    {
        return Handle != nullptr;
    }

    HANDLE* ReleaseAndGetPtr()
    {
        Reset();
        return &Handle;
    }

    HANDLE Handle = nullptr;
};
