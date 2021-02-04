#include "Events.h"
#include "Hooks.h"
#include "IPluginInterface.h"
#include "Logging.h"

class WakingUpNpcs : public IPluginInterface
{
public:
	~WakingUpNpcs() override
	{
		Events::OnConsoleCommand->RemoveListener(m_Listener1);
		Events::OnConsoleCommand->RemoveListener(m_Listener2);
		Events::OnConsoleCommand->RemoveListener(m_Listener3);
	}

	void Init() override
	{
		Logger::Info("Hello world!");

		m_Listener1 = Events::OnConsoleCommand->AddListener([]()
		{
			Logger::Debug("Console command!");
		});

		m_Listener2 = Events::OnConsoleCommand->AddListener([]()
		{
			Logger::Debug("Console command2!");
		});

		m_Listener3 = Events::OnConsoleCommand->AddListener([]()
		{
			Logger::Debug("Console command3!");
		});
	}

private:
	EventDispatcher<void>::EventListener_t m_Listener1;
	EventDispatcher<void>::EventListener_t m_Listener2;
	EventDispatcher<void>::EventListener_t m_Listener3;
};

DECLARE_ZHM_PLUGIN(WakingUpNpcs);