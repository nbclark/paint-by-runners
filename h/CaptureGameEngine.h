#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class CaptureGameEngine : public IGameEngine
{
public:
	CaptureGameEngine();
	~CaptureGameEngine();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	bool ShouldRenderScore();
	int32 GetGameLength() { return 60000; }
	
	virtual char* GetDescription() { return "Capture as many fireflies as you can before time runs out."; }
	virtual CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture, *g_pCursorTexture, *g_pUserTexture;
	CIwTexture* g_pTiles[3][3];
	s3eLocation g_startLocation;
	
	CIwVec2 g_topLeft, g_botRight;
	uint64 g_caughtAllTime;

	CIwArray<s3eLocation*> g_pCaptures;

	CIwFVec2 g_tileLoc;
};