#pragma once
#include "IwUI.h"

typedef void (*TextEntryBoxButtonClick)(void* pParam, const char* szValue);
typedef void (*RenderDelegate)(void* pParam);

class TextEntryBox
{
public:
	static void Show(char* szTitle, char* szDescription, char* szOption1, RenderDelegate pfnRender, TextEntryBoxButtonClick onClicked, void* pParam);
private:
	TextEntryBox(void);
	~TextEntryBox(void);
	void OnOption1(CIwUIElement* Clicked);
	
	void ShowInternal(char* szTitle, char* szDescription, char* szOption1, RenderDelegate pfnRender, TextEntryBoxButtonClick onClicked, void* pParam);

	CIwUIElement* g_pDialog;
	CIwUILabel* g_pTitle;
	CIwUILabel* g_pDescription;
	CIwUIButton* g_pOption1;
	CIwUITextField* g_pTextEntryField;
	CIwUITextInput g_textinput;
	TextEntryBoxButtonClick g_pfnOnClicked;
	void* g_pParam;

	bool g_bIsVisible;

	static TextEntryBox* g_pTextEntryBox;
};