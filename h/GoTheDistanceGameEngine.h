#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class GoTheDistanceGameEngine : public IGameEngine
{
public:
	GoTheDistanceGameEngine();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	bool ShouldRenderScore();
	int16 GetGameId() { return 1; }
	int32 GetGameLength() { return 60000; }
	
	char* GetDescription() { return "Run as far away from your starting point as you can before time runs out.  Distance is measured in meters."; }
	CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture;
	s3eLocation g_startLocation, g_currLoc;
	double g_distance;
	CLineGraph g_lineGraph;
	uint64 g_lastTimeAdd, g_lastTimeCheck;
};