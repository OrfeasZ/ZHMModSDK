#include "Events.h"
#include "Hooks.h"
#include "IPluginInterface.h"
#include "Logging.h"

class WakingUpNpcs : public IPluginInterface
{
public:
	~WakingUpNpcs() override
	{
	}

	void Init() override
	{
		Logger::Info("Hello world!");

		Events::OnConsoleCommand->AddListener(this, [](IPluginInterface*)
		{
			Logger::Debug("Console command!");
		});

		Events::OnConsoleCommand->AddListener(this, [](IPluginInterface*)
		{
			Logger::Debug("Console command2!");
		});

		Events::OnConsoleCommand->AddListener(this, [](IPluginInterface*)
		{
			Logger::Debug("Console command3!");
		});
	}
};

DECLARE_ZHM_PLUGIN(WakingUpNpcs);