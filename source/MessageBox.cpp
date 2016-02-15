#include "MessageBox.h"
#include "Main.h"

MessageBox* MessageBox::g_pMessageBox = NULL;

bool MessageBox::Show(char* szTitle, char* szDescription, char* szOption1, char* szOption2, RenderDelegate pfnRender, void* pParam)
{
	if (NULL == g_pMessageBox)
	{
		g_pMessageBox = new MessageBox();
	}
	return g_pMessageBox->ShowInternal(szTitle, szDescription, szOption1, szOption2, pfnRender, pParam);
}

MessageBox::MessageBox(void)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "MessageBox", MessageBox, OnOption1, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "MessageBox", MessageBox, OnOption2, CIwUIElement*)
	g_pDialog = ((CIwUIElement*)IwGetResManager()->GetResNamed("MessageBox", "CIwUIElement"))->Clone();
	g_pTitle = (CIwUILabel*)g_pDialog->GetChildNamed("Title");
	g_pDescription = (CIwUILabel*)g_pDialog->GetChildNamed("Description");
	g_pOption1 = (CIwUIButton*)g_pDialog->GetChildNamed("Option1");
	g_pOption2 = (CIwUIButton*)g_pDialog->GetChildNamed("Option2");
	
	g_pDialog->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialog);
	IwGetUIView()->AddElementToLayout(g_pDialog);
}

MessageBox::~MessageBox(void)
{
	//
}

bool MessageBox::ShowInternal(char* szTitle, char* szDescription, char* szOption1, char* szOption2, RenderDelegate pfnRender, void* pParam)
{
	g_pTitle->SetCaption(szTitle);
	g_pDescription->SetCaption(szDescription);
	g_pOption1->SetCaption(szOption1);
	g_pOption2->SetCaption(szOption2);
	g_pParam = pParam;

	g_pDialog->SetVisible(true);
	IwGetUIView()->SetModal(g_pDialog);
	IwGetUIAnimManager()->PlayAnim("popupAnim", g_pDialog, false);

	g_bIsVisible = true;
	g_bReturnValue = false;
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
	return g_bReturnValue;
}

void MessageBox::OnOption1(CIwUIElement* Clicked)
{
	g_bIsVisible = false;

	IwGetUIView()->SetModal(NULL);
	g_pDialog->SetVisible(false);
	g_bReturnValue = true;
}

void MessageBox::OnOption2(CIwUIElement* Clicked)
{
	g_bIsVisible = false;

	IwGetUIView()->SetModal(NULL);
	g_pDialog->SetVisible(false);
	g_bReturnValue = false;
}