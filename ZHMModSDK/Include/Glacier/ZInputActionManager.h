#include "Reflection.h"
#include "ZGameTime.h"

class ZActionMapTree;

class ZInputActionManager : IComponentInterface
{
public:
	ZActionMapTree* m_pkRoot;
	ZActionMapTree* m_pkCurrentBlock;
	int m_iBindMem;
	bool m_bDebugKeys;
	bool m_bEnabled;
	bool m_bSpeedRepeat;
	bool m_JoinControllers;
	ZGameTime m_EventHorizonDelay;
};
