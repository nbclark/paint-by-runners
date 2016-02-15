#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class EvasionGameEngine : public IGameEngine
{
public:
	EvasionGameEngine();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	bool ShouldRenderScore();
	
	virtual char* GetDescription() { return "Evade capture for as long as you can. If the captors get within 5 meters of you, you will be captured."; }
	virtual CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture;
	s3eLocation g_startLocation;
	double g_distance;
	CLineGraph g_lineGraph;
	uint64 g_lastTimeAdd;
};