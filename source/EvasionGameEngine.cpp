#pragma once
#include "EvasionGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"

EvasionGameEngine::EvasionGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("Evasion", "CIwTexture");
}

void EvasionGameEngine::Start()
{
	g_lastTimeAdd = -1000;
	g_lineGraph.Clear();
	g_distance = 0;
	Utils::GetLocation(&g_startLocation);
	IwRandSeed(s3eTimerGetMs());
}

float EvasionGameEngine::End()
{
	s3eLocation endLoc;
	Utils::GetLocation(&endLoc);

	g_lineGraph.Clear();
	
	double distance = LiveMaps::CalculateDistance(g_startLocation, endLoc);
	return (float)distance;
}

void EvasionGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	s3eLocation curLoc;
	Utils::GetLocation(&curLoc);

	double distance = LiveMaps::CalculateDistance(g_startLocation, curLoc);
	g_distance += (double)(IwRandMinMax(-1000, 5000) * .000111);
	*pfScore = (float)g_distance;

	if ((g_lastTimeAdd - remainingTime) > 3000)
	{
		g_lastTimeAdd = remainingTime;
		g_lineGraph.AddPoint(*pfScore);
	}

	strcpy(szUnits, "meters");
}

void EvasionGameEngine::Render(CIwRect& bounds)
{
	CIwSVec2 imgBounds(bounds.x, bounds.y);
	Utils::AlphaRenderImage(g_pLogoTexture, imgBounds, 0xff);

	CIwRect setBounds(bounds.x, bounds.y + g_pLogoTexture->GetHeight(), bounds.w, bounds.h - g_pLogoTexture->GetHeight());
	g_lineGraph.SetBounds(setBounds);
	g_lineGraph.Render();
}

bool EvasionGameEngine::ShouldRenderScore()
{
	return true;
}