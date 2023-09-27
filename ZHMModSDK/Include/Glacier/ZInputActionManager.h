#include "Reflection.h"
#include "ZGameTime.h"
#include "ZInput.h"
#include <Functions.h>

class ZActionMapTree;

class ZInputActionManager : IComponentInterface
{
public:
	static bool AddBindings(const char* binds)
	{
		ZInputTokenStream stream;
		stream.m_szData = binds;

		*Globals::InputActionManager_BindMem += strlen(binds);

		Functions::ZInputTokenStream_ParseToken->Call(&stream, &stream.kaTokens[0]);
		
		Globals::InputActionManager->m_pkCurrentBlock = Globals::InputActionManager->m_pkRoot;

		bool result = Functions::ZInputActionManager_ParseAsignment->Call(Globals::InputActionManager, &stream);

		if (!result)
		{
			return false;
		}

		++*Globals::InputActionManager_Seq;

		return true;
	}

	ZActionMapTree* m_pkRoot;
	ZActionMapTree* m_pkCurrentBlock;
	int m_iBindMem;
	bool m_bDebugKeys;
	bool m_bEnabled;
	bool m_bSpeedRepeat;
	bool m_JoinControllers;
	ZGameTime m_EventHorizonDelay;
};
