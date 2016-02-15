#pragma once
#include "gamestate.h"
#include "IwMultiplayerHandler.h"

class SelectGameGameState :
	public GameState
{
public:
	SelectGameGameState(IGameHandler* pGameHandler);
	~SelectGameGameState(void);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();

private:
	void OnClickStartMulti(CIwUIElement* Clicked);
	void OnClickStartSingle(CIwUIElement* Clicked);
	void OnClickBack(CIwUIElement* Clicked);
	void OnClickUser(CIwUIElement* Clicked);
	void OnClickDelete(CIwUIElement* Clicked);

	static void MultiplayerModeChanged(MultiplayerMode mode, void* userData);
	static void CreateStatus(bool success, const char* szStatus, void* userData);
	static void JoinStatus(bool success, const char* szStatus, void* userData);

	CIwUIButton* g_pCreateButton;
	CIwUIButton* g_pStartButton;
	bool g_bWaitingOnGps, g_bWaitForStart, g_bWaitForCreate;
	CIwArray<CIwUIElement*> g_userButtons;

	CIwTexture* g_pUserTexture;
	CIwTexture* g_pUserTextureB;
	CIwTexture* g_pBackground;
};

