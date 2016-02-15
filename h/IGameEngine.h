#pragma once
#include "IwUI.h"
#include "IwGx.h"
#include "Utils.h"

class IGameEngine
{
public:
	virtual int16 GetGameId() { return 0; }
	virtual int32 GetGameLength() { return 60000; }
	virtual void Prepare() {}
	virtual void Start() {}
	virtual float End() { return 0; }
	virtual void Update(uint64 remainingTime, float* pfScore, char szUnits[100]) {}
	virtual void Render(CIwRect& bounds) {}
	virtual void RenderInfo(CIwRect& bounds)
	{
		CIwTexture* pLogo = GetLogo();
		int logoHeight = 0;

		if (pLogo)
		{
			logoHeight = pLogo->GetHeight();

			CIwSVec2 draw(bounds.x + (bounds.w - pLogo->GetWidth()) / 2, bounds.y);
			Utils::AlphaRenderImage(pLogo, draw, 0xff);
		}

		CIwRect textBounds(bounds.x + 2, bounds.y + 2 + logoHeight, bounds.w - 4, bounds.h - 4 - logoHeight);
		CIwGxFont* pFont = (CIwGxFont*)IwGetResManager()->GetResNamed("font_medium", "CIwGxFont");
		IwGxLightingOn();
		IwGxFontSetFont(pFont);
		IwGxFontSetCol(0xffffffff);
		IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_TOP);
		IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);

		IwGxFontSetRect(textBounds);
		IwGxFontDrawText(GetDescription());

		IwGxLightingOff();
	}
	virtual char* GetDescription() { return "FILL IN HERE..."; }
	virtual CIwTexture* GetLogo() { return NULL; }
	virtual bool ShouldRenderScore() { return true; }
	
	void SetGameHandler(void* pGameHandler) { g_pGameHandler = pGameHandler; }
protected :
	void* g_pGameHandler;
};