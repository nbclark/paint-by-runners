#include "TextEntryBox.h"
#include "Main.h"

TextEntryBox* TextEntryBox::g_pTextEntryBox = NULL;

void TextEntryBox::Show(char* szTitle, char* szDescription, char* szOption1, RenderDelegate pfnRender, TextEntryBoxButtonClick onClicked, void* pParam)
{
	if (NULL == g_pTextEntryBox)
	{
		g_pTextEntryBox = new TextEntryBox();
	}
	g_pTextEntryBox->ShowInternal(szTitle, szDescription, szOption1, pfnRender, onClicked, pParam);
}

TextEntryBox::TextEntryBox(void)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "TextEntryBox", TextEntryBox, OnOption1, CIwUIElement*)
	g_pDialog = ((CIwUIElement*)IwGetResManager()->GetResNamed("TextEntryBox", "CIwUIElement"))->Clone();
	g_pTitle = (CIwUILabel*)g_pDialog->GetChildNamed("Title");
	g_pDescription = (CIwUILabel*)g_pDialog->GetChildNamed("Description");
	g_pOption1 = (CIwUIButton*)g_pDialog->GetChildNamed("Option1");
	g_pTextEntryField = (CIwUITextField*)g_pDialog->GetChildNamed("TextEntryField");
	
	g_pDialog->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialog);
	IwGetUIView()->AddElementToLayout(g_pDialog);
	g_textinput.CreateSoftKeyboard();
	//g_textinput.SetEditorMode(CIwUITextInput::EDITOR_OS);
	g_textinput.SetEditorMode(CIwUITextInput::EDITOR_KEYBOARD);
}

TextEntryBox::~TextEntryBox(void)
{
	//
}

void TextEntryBox::ShowInternal(char* szTitle, char* szDescription, char* szOption1, RenderDelegate pfnRender, TextEntryBoxButtonClick onClicked, void* pParam)
{
	g_pTitle->SetCaption(szTitle);
	g_pDescription->SetCaption(szDescription);
	g_pOption1->SetCaption(szOption1);
	g_pfnOnClicked = onClicked;
	g_pParam = pParam;

	g_pDialog->SetVisible(true);
	IwGetUIView()->SetModal(g_pDialog);
	IwGetUIAnimManager()->PlayAnim("popupAnim", g_pDialog, false);
	
	g_bIsVisible = true;
	while (g_bIsVisible)
	{
		int64 start = s3eTimerGetMs();
		IwGxClear(IW_GX_DEPTH_BUFFER_F);

		if (pfnRender)
		{
			pfnRender(pParam);
		}
		IwGetUIController()->Update();
		IwGetUIView()->Update(1000/20);
		IwGetUIView()->Render();

		IwGxFlush();
		IwGxSwapBuffers();

		// Attempt frame rate
		while ((s3eTimerGetMs() - start) < MS_PER_FRAME)
		{
			int32 yield = (int32) (MS_PER_FRAME - (s3eTimerGetMs() - start));
			if (yield<0)
			{
				break;
			}
			s3eDeviceYield(yield);
		}
	}
}

void TextEntryBox::OnOption1(CIwUIElement* Clicked)
{
	g_bIsVisible = false;

	const char* szValue = g_pTextEntryField->GetCaption();

	IwGetUIView()->SetModal(NULL);
	g_pDialog->SetVisible(false);
	g_pfnOnClicked(g_pParam, szValue);
}