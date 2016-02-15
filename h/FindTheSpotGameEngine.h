#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class FindTheSpotGameEngine : public IGameEngine
{
public:
	FindTheSpotGameEngine();
	void Prepare();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	bool ShouldRenderScore();
	int32 GetGameLength() { return 60000; }
	int16 GetGameId() { return 2; }
	
	virtual char* GetDescription() { return "Follow the clues of hotter or colder to find the spot marked on the map.  Do it quickly before you run out of time."; }
	virtual CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture, *g_pTile, *g_pHotterTexture, *g_pColderTexture, *g_pCursorTexture, *g_pFoundTexture;
	s3eLocation g_startLocation;
	double g_distance, g_finalScore;
	uint64 g_lastTimeCheck;

	int g_renderTemp;

	s3eLocation g_topLeft, g_botRight, g_randLocation, g_currLoc;
	CIwFVec2 g_tileLoc;
};