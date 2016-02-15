#pragma once
#include "gamestate.h"
#include "IwUI.h"
#include <stdlib.h>
#include <stdio.h>
#include <list>

struct ButtonHash
{
	CIwUIElement* pElement;
	CIwUIElement* pTemplate;
	char* szPath;
	float fDistance;
};

struct ButtonHashSorter
{
  bool operator()(const ButtonHash* __x, const ButtonHash* __y) const { return __x->fDistance <= __y->fDistance; }
};

class SelectMapGameState :
	public GameState
{
public:
	SelectMapGameState(void);
	~SelectMapGameState(void);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();
private:
	void ClearLoadedMaps(bool removeButtons);
	void OnClickMapItem(CIwUIElement* Clicked);
	void OnClickDelete(CIwUIElement* Clicked);
	void OnClickBack(CIwUIElement* Clicked);
	bool LoadMap(char* szMap, CIwArray<s3eLocation>* pPoints, float* pZoom);
	static void DeleteMessageBoxClosed(void * pParam, bool button1);

	char* g_szDeleteFile;
	bool g_firstSelectShow;
	std::list<CIwUIElement*> g_savedMaps;
	CIwArray<s3eLocation> g_loadedCorners;
	std::list<ButtonHash*> g_loadedMaps;
	CIwUIButton* g_pCreateButton;
	CIwUIButton* g_pTiltButton;

	CIwTexture* g_pBackground;
};

