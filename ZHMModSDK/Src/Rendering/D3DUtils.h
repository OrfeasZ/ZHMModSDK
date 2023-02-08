#pragma once

#define REF_IID_PPV_ARGS(Val) IID_PPV_ARGS(&Val.Ref)

namespace Rendering
{
	template <class T>
	struct ScopedD3DRef
	{
		typedef T* RefType;

		ScopedD3DRef(RefType p_Ref) : Ref(p_Ref) {}

		ScopedD3DRef() : Ref(nullptr) {}

		~ScopedD3DRef()
		{
			if (Ref)
				Ref->Release();
		}

		void Reset()
		{
			if (Ref)
				Ref->Release();

			Ref = nullptr;
		}

		RefType operator->()
		{
			return Ref;
		}

		operator RefType()
		{
			return Ref;
		}

		operator bool()
		{
			return Ref != nullptr;
		}

		void* VTable()
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
}

struct SafeHandle
{
	SafeHandle(HANDLE p_Handle) : Handle(p_Handle) {}

	SafeHandle() : Handle(nullptr) {}

	~SafeHandle()
	{
		if (Handle)
			CloseHandle(Handle);
	}

	void Reset()
	{
		if (Handle)
			CloseHandle(Handle);

		Handle = nullptr;
	}
	
	operator HANDLE()
	{
		return Handle;
	}

	operator bool()
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
