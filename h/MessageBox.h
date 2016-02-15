#pragma once
#include "IwUI.h"

typedef void (*RenderDelegate)(void* pParam);

class MessageBox
{
public:
	static bool Show(char* szTitle, char* szDescription, char* szOption1, char* szOption2, RenderDelegate pfnRender, void* pParam);
private:
	MessageBox(void);
	~MessageBox(void);
	void OnOption1(CIwUIElement* Clicked);
	void OnOption2(CIwUIElement* Clicked);
	
	bool ShowInternal(char* szTitle, char* szDescription, char* szOption1, char* szOption2, RenderDelegate pfnRender, void* pParam);

	CIwUIElement* g_pDialog;
	CIwUILabel* g_pTitle;
	CIwUILabel* g_pDescription;
	CIwUIButton* g_pOption1;
	CIwUIButton* g_pOption2;
	void* g_pParam;

	bool g_bIsVisible, g_bReturnValue;

	static MessageBox* g_pMessageBox;
};