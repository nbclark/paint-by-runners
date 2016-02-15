#pragma once
#include "gamestate.h"
#include "Utils.h"
#include "MapBackground.h"
class CreateMapGameState :
	public GameState
{
public:
	CreateMapGameState(void);
	~CreateMapGameState(void);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();
private:
	void RenderCursor();
	void RenderClicks();
	void RenderBackground();
	static void MessageBoxClosed(void * pParam, bool button1);
	static void TextEntryClosed(void * pParam, const char* szText);
	static void MessageRenderBackground(void * pParam);

	void OnClickBack(CIwUIElement* Clicked);
	
	std::list<s3eLocation> g_downPos;

	CIwSVec2 g_location;

	int g_cursorIter;
	CIw2DImage* gCursor, *gMark;
	MapBackground* g_pBackground;
	Region g_region;
	CIwArray<s3eLocation> g_points;
};

