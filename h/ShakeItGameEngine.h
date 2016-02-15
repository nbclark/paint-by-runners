#pragma once
#include "IGameObjects.h"
#include "IGameEngine.h"
#include "s3eLocation.h"

class ShakeItGameEngine : public IGameEngine
{
public:
	ShakeItGameEngine();
	void Start();
	float End();
	void Update(uint64 remainingTime, float* pfScore, char szUnits[100]);
	void Render(CIwRect& bounds);
	void RenderInfo(CIwRect& bounds);
	bool ShouldRenderScore();
	int32 GetGameLength() { return 30000; }
	int16 GetGameId() { return 3; }
	
	virtual char* GetDescription() { return "Shake your phone as quickly as you can up, down, left or right.  The most G's wins."; }
	virtual CIwTexture* GetLogo() { return g_pLogoTexture; }

private:
	CIwTexture* g_pLogoTexture;

	float g_maxShake;
	int g_iAnimIter;
};