#pragma once

class DiscordClient
{
public:
	void Initialize();
	void Update(const char* state, const char* details, const char* imageKey);
	void Teardown();
};
