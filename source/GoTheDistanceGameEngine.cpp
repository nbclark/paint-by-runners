#pragma once
#include "GoTheDistanceGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"

GoTheDistanceGameEngine::GoTheDistanceGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("gothedistance", "CIwTexture");
}

void GoTheDistanceGameEngine::Start()
{
	g_lastTimeAdd = -1000;
	g_lineGraph.Clear();
	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());
	
	g_lineGraph.SetTotalPoints(this->GetGameLength() / 2000);
}

float GoTheDistanceGameEngine::End()
{
	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	g_lineGraph.Clear();
	
	double distance = LiveMaps::CalculateDistance(g_startLocation, endLoc);
	return (float)distance;
}

void GoTheDistanceGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	if (ABS(g_lastTimeCheck - remainingTime) > 500)
	{
		g_lastTimeCheck = remainingTime;
		Utils::GetLocation(&g_currLoc);
	}

	double distance = LiveMaps::CalculateDistance(g_startLocation, g_currLoc);
	*pfScore = (float)distance;

	if ((g_lastTimeAdd - remainingTime) > 2000)
	{
		g_lastTimeAdd = remainingTime;
		g_lineGraph.AddPoint(*pfScore);
	}

	strcpy(szUnits, "meters");
}

void GoTheDistanceGameEngine::Render(CIwRect& bounds)
{
	CIwSVec2 imgBounds(bounds.x, bounds.y);
	Utils::AlphaRenderImage(g_pLogoTexture, imgBounds, 0xff);

	CIwRect setBounds(bounds.x, bounds.y + g_pLogoTexture->GetHeight(), bounds.w, bounds.h - g_pLogoTexture->GetHeight());
	g_lineGraph.SetBounds(setBounds);
	g_lineGraph.Render();
}

bool GoTheDistanceGameEngine::ShouldRenderScore()
{
	return true;
}