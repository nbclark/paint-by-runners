#pragma once
#include "EggDropGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"
#include "s3e.h"

float rot, fallingRot;
float g_finalScore = 0;
bool g_bBroken = false;
int g_iCrackIter = 0;

EggDropGameEngine::EggDropGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("eggdrop", "CIwTexture");
	g_pEgg = (CIwTexture*)IwGetResManager()->GetResNamed("egg", "CIwTexture");
	g_pSpoon = (CIwTexture*)IwGetResManager()->GetResNamed("spoon", "CIwTexture");
	g_pCrackedEgg = (CIwTexture*)IwGetResManager()->GetResNamed("crackedegg", "CIwTexture");
	g_pGrass = (CIwTexture*)IwGetResManager()->GetResNamed("grass", "CIwTexture");
}

void EggDropGameEngine::Start()
{
	g_lastTimeAdd = -1000;
	g_lineGraph.Clear();
	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());
	
	s3eAccelerometerStart();
	g_bBroken = false;
	g_finalScore = 0;
	g_iCrackIter = 0;
}

float EggDropGameEngine::End()
{
	s3eAccelerometerStop();

	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	g_lineGraph.Clear();
	
	double distance = LiveMaps::CalculateDistance(g_startLocation, endLoc);
	return (float)distance;
}

void EggDropGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	s3eLocation curLoc;
	Utils::GetLocation(&curLoc);

	int requiredDistance = 50;

	double distance = LiveMaps::CalculateDistance(g_startLocation, curLoc);

	// Get the accelerometer values here...
	int32 x = s3eAccelerometerGetX();
	int32 y = s3eAccelerometerGetY();
	int32 z = s3eAccelerometerGetZ();

	if (g_finalScore != 0)
	{
		*pfScore = 0;
	}
	else if (g_bBroken)
	{
		*pfScore = 0;
		g_finalScore = -1;
	}
	else if (ABS(x) > 450)
	{
		*pfScore = 0;

		// we break;
		g_bBroken = true;
		g_iCrackIter = 0;
		fallingRot = rot;

		g_finalScore = (distance - requiredDistance);
	}
	else if (distance > requiredDistance)
	{
		g_finalScore = (GetGameLength() - remainingTime);
		*pfScore = 0;
	}
	else
	{
		*pfScore = (requiredDistance - distance);
		rot = -(x / 1000.0) / 2;
	}

	strcpy(szUnits, "meters remaining");
}

void EggDropGameEngine::RenderInfo(CIwRect& bounds)
{
	IGameEngine::RenderInfo(bounds);
}

void EggDropGameEngine::Render(CIwRect& bounds)
{
	CIwSVec2 drawGrass(bounds.x + (bounds.w - g_pGrass->GetWidth()) / 2, bounds.y);
	Utils::AlphaRenderImage(g_pGrass, drawGrass, 0xff);

	if (g_bBroken)
	{
		if (g_iCrackIter < 40)
		{
			int yOffset = (g_pSpoon->GetHeight() - g_pEgg->GetHeight()) * g_iCrackIter / 40.0;
			CIwRect bounds2(bounds.x + (bounds.w - g_pEgg->GetWidth()) / 2, bounds.y + yOffset, g_pEgg->GetWidth(), g_pEgg->GetHeight());

			double fallRot = fallingRot + (g_iCrackIter * 2 / 40.0);

			Utils::AlphaRenderAndRotateImage(g_pEgg, bounds2, 0xff, fallRot);

			g_iCrackIter++;
		}
		else
		{
			CIwSVec2 draw(bounds.x + (bounds.w - g_pCrackedEgg->GetWidth()) / 2, bounds.y + (bounds.h - g_pCrackedEgg->GetHeight()));
			Utils::AlphaRenderImage(g_pCrackedEgg, draw, 0xff);
		}
	}
	else
	{
		CIwSVec2 draw(bounds.x + (bounds.w - g_pSpoon->GetWidth()) / 2, bounds.y);
		Utils::AlphaRenderImage(g_pSpoon, draw, 0xff);

		IwGxFlush();

		CIwRect bounds2(bounds.x + (bounds.w - g_pEgg->GetWidth()) / 2, bounds.y, g_pEgg->GetWidth(), g_pEgg->GetHeight());
		Utils::AlphaRenderAndRotateImage(g_pEgg, bounds2, 0xff, rot);
	}
}

bool EggDropGameEngine::ShouldRenderScore()
{
	return true;
}