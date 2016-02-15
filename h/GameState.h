#pragma once
#include "IGameState.h"
#include "MapGame.h"

class GameState : public IGameState
{
public:
	GameState(void);
	~GameState(void);

	void SetGameHandler(IGameHandler* pMapGame);
	IGameHandler* GetGameHandler();
	virtual void Update();
	virtual void Render();

	virtual void Activate();
	virtual void DeActivate();

protected:
	static void MessageRenderBackground(void * pParam)
	{
		IGameState* pGame = (IGameState*)pParam;
	}

	IGameHandler* g_pMapGame;
	CIwUIElement* g_pDialogMain;
};
