#pragma once
#include "IGameEngine.h"
#include "IPongObjects.h"

class PongGameEngine :
	public IGameEngine
{
public:
	PongGameEngine(IGameHandler* pGameState);
	~PongGameEngine(void);

	virtual bool Update(uint64 uiGameTimer, bool* pbVictory);
	virtual void RenderGX();
	virtual void Render2D();
	virtual void RenderStatus(CIwUIRect& bounds) {}

	virtual void Activate();
	virtual void DeActivate();

private:
	CBall* g_pBall;
	IGameObject* g_pUserPaddle;
	IGameObject* g_pCompPaddle;
};
