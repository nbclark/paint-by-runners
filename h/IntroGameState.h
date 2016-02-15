#pragma once
#include "gamestate.h"
class IntroGameState :
	public GameState
{
public:
	IntroGameState(IGameHandler* pGameHandler);
	~IntroGameState(void);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();
private:
	void OnClickStartGame(CIwUIElement* Clicked);
	void OnClickLoadMap(CIwUIElement* Clicked);
	void OnClickHighScores(CIwUIElement* Clicked);
	void OnClickHelp(CIwUIElement* Clicked);
	void OnClickExit(CIwUIElement* Clicked);

	CIwUIButton* g_pStartButton;
	CIwUIButton* g_pLoadMapButton;
	CIwUIImage* g_pConnectedImage;
	CIwUIElement* g_pConnectingElem;
	CIwUILabel* g_pConnectingLabel;
	bool g_bWaitingOnGps;
	CIwTexture* g_pTextureFirefly, *g_pBackground;
};

