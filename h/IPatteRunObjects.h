#pragma once
#include "IGameObject.h"

// build a grid of grid objects...light up certain tiles. Have a user object & light up grid under user

class CGridTile : public IGameObject
{
public:
	CGridTile(IGameHandler* pGameState, CIwRect& bounds, bool bDark) : IGameObject(pGameState)
	{
		g_location.x = bounds.x;
		g_location.y = bounds.y;
		g_width = bounds.w;
		g_height = bounds.h;

		g_bRadialInsersection = false;
		g_bActive = false;
		g_bDark = bDark;
	}
	bool IsContained(CIwVec2* pPos)
	{
		if (pPos->x >= g_location.x && pPos->x < (g_location.x + g_width))
		{
			if (pPos->y >= g_location.y && pPos->y < (g_location.y + g_height))
			{
				return true;
			}
		}
		return false;
	}
	void SetActive(bool bActive)
	{
		g_bActive = bActive;
	}
	bool Update(bool* pbVictory)
	{
		return false;
	}
	void Render()
	{
		CIwColour col;
		if (g_bActive)
		{
			col.Set(0xFf, 0xFF, 0xFF, 125);
		}
		else if (g_bDark)
		{
			col.Set(0, 0, 0, 150);
		}
		else
		{
			col.Set(0, 0, 0, 50);
		}
		Iw2DSetColour(col);
		Iw2DFillRect(CIwSVec2((int16)g_location.x, (int16)g_location.y), CIwSVec2((int16)g_width, (int16)g_height));
	}
private:
	bool g_bActive, g_bDark;
};
