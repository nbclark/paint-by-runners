#include "StaticContentGameState.h"
#include "tinyxml.h"
#include "MessageBox.h"

StaticContentGameState::StaticContentGameState(void)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "StaticContentGameState", StaticContentGameState, OnClickBack, CIwUIElement*)

	g_pDialogMain = NULL;
}

StaticContentGameState::~StaticContentGameState(void)
{
	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void StaticContentGameState::SetContent(char* panelName)
{
	if (g_pDialogMain)
	{
		IwGetUIView()->RemoveElement(g_pDialogMain);
	}

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed(panelName, "CIwUIElement");
	g_pDialogMain->SetVisible(false);

	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);
}

void StaticContentGameState::PerformActivate()
{
	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);
}

void StaticContentGameState::PerformDeActivate()
{
}

void StaticContentGameState::PerformUpdate()
{
	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);
}

void StaticContentGameState::PerformRender()
{
	IwGxSetScreenSpaceSlot(-1);
	IwGxFlush();
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();
}

void StaticContentGameState::OnClickBack(CIwUIElement* Clicked)
{
	g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Left);
}