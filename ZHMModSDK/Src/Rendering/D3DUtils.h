#pragma once

#define REF_IID_PPV_ARGS(Val) IID_PPV_ARGS(&Val.Ref)

namespace Rendering
{
	template <class T>
	struct ScopedD3DRef
	{
	public:
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

		void* VTable()
		{
			return *reinterpret_cast<void**>(Ref);
		}

		RefType Ref;
	};
}
