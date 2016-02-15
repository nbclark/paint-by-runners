#include "PongGameEngine.h"
#include "MessageBox.h"

PongGameEngine::PongGameEngine(IGameHandler* pGameState)
{
	g_pBall = new CBall(pGameState);
	g_pUserPaddle = new CGPSPaddle(pGameState);
	//g_pUserPaddle = new CCompPaddle(this, g_pBall, false);
	g_pCompPaddle = new CCompPaddle(pGameState, g_pBall, true);

	g_pBall->PushPaddle(g_pUserPaddle);
	g_pBall->PushPaddle(g_pCompPaddle);
}

PongGameEngine::~PongGameEngine(void)
{
	delete g_pBall;
	delete g_pUserPaddle;
	delete g_pCompPaddle;
}

void PongGameEngine::Activate()
{
	g_pBall->Reset();
	g_pCompPaddle->Reset();
	g_pUserPaddle->Reset();
}

void PongGameEngine::DeActivate()
{
	//
}

bool PongGameEngine::Update(uint64 uiGameTimer, bool* pbVictory)
{
	g_pUserPaddle->Update(pbVictory);

	// Update the ball movement here & do collision detection
	g_pCompPaddle->Update(pbVictory);
	
	if (g_pBall->Update(pbVictory))
	{
		return true;
	}

	return false;
}

void PongGameEngine::RenderGX()
{
}

void PongGameEngine::Render2D()
{
	g_pUserPaddle->Render();

	// Update the ball movement here & do collision detection
	g_pCompPaddle->Render();
	g_pBall->Render();
}