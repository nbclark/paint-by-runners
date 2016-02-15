#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class EggDropGameEngine : public IGameEngine
{
public:
	EggDropGameEngine();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	void RenderInfo(CIwRect& bounds);
	bool ShouldRenderScore();
	int32 GetGameLength() { return 45000; }
	int16 GetGameId() { return 3; }
	
	virtual char* GetDescription() { return "Carry the egg on the spoon for 50 meters.  Keep the phone's screen face up and be careful not to tilt it too much, or the egg will drop."; }
	virtual CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture;
	CIwTexture* g_pEgg;
	CIwTexture* g_pSpoon;
	CIwTexture* g_pCrackedEgg;
	CIwTexture* g_pGrass;
	s3eLocation g_startLocation;
	double g_distance;
	CLineGraph g_lineGraph;
	uint64 g_lastTimeAdd;
};