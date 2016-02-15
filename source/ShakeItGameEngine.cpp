#pragma once
#include "ShakeItGameEngine.h"
#include "Utils.h"
#include "LiveMaps.h"
#include "s3e.h"

#define SHAKEIT_MAXSHAKE	10000.0
#define SHAKEIT_BARHEIGHT	15
#define SHAKEIT_BARSPACE	5

ShakeItGameEngine::ShakeItGameEngine()
{
	g_pLogoTexture = (CIwTexture*)IwGetResManager()->GetResNamed("shakeit", "CIwTexture");
}

void ShakeItGameEngine::Start()
{
	g_maxShake = 0;
	g_iAnimIter = 0;
	
	s3eAccelerometerStart();
}

float ShakeItGameEngine::End()
{
	s3eAccelerometerStop();

	return g_maxShake;
}

void ShakeItGameEngine::Update(uint64 remainingTime, float* pfScore, char szUnits[100])
{
	// Get the accelerometer values here...
	int32 x = s3eAccelerometerGetX();
	int32 y = s3eAccelerometerGetY();
	int32 z = s3eAccelerometerGetZ();

	g_maxShake = MAX(g_maxShake, (ABS(x) + ABS(y) + ABS(z)));
	*pfScore = g_maxShake;

	g_iAnimIter = (g_iAnimIter + 1) % 41;

	strcpy(szUnits, "shake strength");
}

void ShakeItGameEngine::RenderInfo(CIwRect& bounds)
{
	IGameEngine::RenderInfo(bounds);
}

void ShakeItGameEngine::Render(CIwRect& bounds)
{
	CIwSVec2 rectBounds(bounds.x + 10, bounds.y + bounds.h - 10 - 20);
	CIwSVec2 size(bounds.w - 20, SHAKEIT_BARHEIGHT);

	int renderedHeight = 0;
	int shakeHeight = (bounds.h - 20);

	Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
	CIwColour black;
	black.Set(0, 0, 0, 0x7f);
	Iw2DSetColour(black);
	
	while ((renderedHeight + 20) < shakeHeight)
	{
		Iw2DFillRect(rectBounds, size);
		renderedHeight += (SHAKEIT_BARHEIGHT + SHAKEIT_BARSPACE);
		rectBounds.y -= (SHAKEIT_BARHEIGHT + SHAKEIT_BARSPACE);
	}

	float maxShake = MIN(SHAKEIT_MAXSHAKE, g_maxShake);
	shakeHeight = (bounds.h - 20) * ((maxShake / SHAKEIT_MAXSHAKE) * (g_iAnimIter / 40.0));
	renderedHeight = 0;

	Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);
	CIwColour neonGreen;
	neonGreen.Set(76, 255, 0);

	rectBounds.x = bounds.x + 10;
	rectBounds.y = bounds.y + bounds.h - 10 - 20;

	Iw2DSetColour(neonGreen);
	while ((renderedHeight + SHAKEIT_BARHEIGHT) < shakeHeight)
	{
		Iw2DFillRect(rectBounds, size);
		renderedHeight += (SHAKEIT_BARHEIGHT + SHAKEIT_BARSPACE);
		rectBounds.y -= (SHAKEIT_BARHEIGHT + SHAKEIT_BARSPACE);
	}
}

bool ShakeItGameEngine::ShouldRenderScore()
{
	return true;
}