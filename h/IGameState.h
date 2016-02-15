#pragma once
#include "IGameHandler.h"

class IGameState
{
public:
	virtual ~IGameState()
	{
		int x = 0;
		g_tickCount = 0;
	}
	virtual void SetGameHandler(IGameHandler* pMapGame) {}
	virtual IGameHandler* GetGameHandler() { return NULL; }
	void Update()
	{
		g_tickCount++;
		PerformUpdate();
	}
	void Render()
	{
		PerformRender();
	}

	void Activate()
	{
		g_tickCount = 0;
		PerformActivate();
	}
	void DeActivate()
	{
		PerformDeActivate();
	}

protected :

	virtual void PerformUpdate() {}
	virtual void PerformRender() {}
	virtual void PerformActivate() {}
	virtual void PerformDeActivate() {}

	long g_tickCount;
};