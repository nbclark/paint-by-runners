#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class BoomerangGameEngine : public IGameEngine
{
public:
	BoomerangGameEngine();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	bool ShouldRenderScore();
	int32 GetGameLength() { return 60000; }
	
	char* GetDescription() { return "Run as far away from your starting point as you can, but you must make it back to your starting point before time runs out.  Distance is measured in meters."; }
	CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture;
	s3eLocation g_startLocation;
	double g_distance;
	double g_maxDistance;
	CLineGraph g_lineGraph;
	uint64 g_lastTimeAdd;
};