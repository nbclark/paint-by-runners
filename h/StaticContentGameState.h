#pragma once
#include "gamestate.h"
#include "IwUI.h"
#include <stdlib.h>
#include <stdio.h>
#include <list>

class StaticContentGameState :
	public GameState
{
public:
	StaticContentGameState(void);
	~StaticContentGameState(void);

	void SetContent(char* panelName);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();

private:
	void OnClickBack(CIwUIElement* Clicked);

	CIwUIElement* g_pDialogMain;
};

