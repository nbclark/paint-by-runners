#pragma once
#include "IGameEngine.h"
#include "IPatteRunObjects.h"
#include "AccelerometerHandler.h"

class PatteRunGameEngine :
	public IGameEngine
{
public:
	PatteRunGameEngine();
	~PatteRunGameEngine(void);

	virtual void Init(void* pGameState);
	virtual bool Update(uint64 uiGameTimer, bool* pbVictory);
	virtual void RenderGX();
	virtual void Render2D();
	virtual void RenderStatus(CIwUIRect& bounds);
	virtual bool RenderPause();

	virtual void Activate();
	virtual void DeActivate();

	virtual bool UpdateLevel();
	virtual int GetScore();

	virtual char* GetHighScoreFile() { return "patterun.xml"; }

private:
	char g_szStatus[100];
	int g_pCoords[20];
	int g_iRows, g_iCols, g_iCounter, g_iGameCounter;
	std::list<CGridTile*> g_pTiles;
	IGameHandler* g_pGameHandler;
	CGPSUser* g_pUser;
	int g_iLevel;
	int g_iScore;
	int g_iDisplayedScore;
	CIwGxFont* g_pFont;
	CIwGxFont* g_pFontHuge;
	CIwGxFont* g_pFontSmall;
	CIwTexture* g_pTile;

	AccelerometerHandler g_handler;
};
