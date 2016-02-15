#include "IntroGameState.h"
#include "Utils.h"

IntroGameState::IntroGameState(IGameHandler* pGameHandler)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "IntroGameState", IntroGameState, OnClickStartGame, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "IntroGameState", IntroGameState, OnClickLoadMap, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "IntroGameState", IntroGameState, OnClickHighScores, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "IntroGameState", IntroGameState, OnClickHelp, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "IntroGameState", IntroGameState, OnClickExit, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("main\\panel", "CIwUIElement");
	g_pStartButton = (CIwUIButton*)g_pDialogMain->GetChildNamed("Button_StartGame");
	g_pLoadMapButton = (CIwUIButton*)g_pDialogMain->GetChildNamed("Button_LoadMap");
	g_pConnectedImage = (CIwUIImage*)g_pDialogMain->GetChildNamed("Image_Connected");
	g_pConnectingElem = (CIwUIElement*)g_pDialogMain->GetChildNamed("Label_Connecting");
	g_pConnectingLabel = (CIwUILabel*)g_pConnectingElem->GetChildNamed("Text");
	
	g_pBackground = (CIwTexture*)IwGetResManager()->GetResNamed("introback", "CIwTexture");

	g_pDialogMain->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	g_bWaitingOnGps = true;
}

IntroGameState::~IntroGameState(void)
{
	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void IntroGameState::PerformUpdate()
{
	bool bEnd;
	if ((g_tickCount % 10) == 0)
	{
		if (!g_pMapGame->IsGpsActive())
		{
			if (g_tickCount % 2 == 0)
			{
				CIwUIMat rotMat;
				rotMat.SetIdentity();
				double perCentAngle = (g_tickCount / 360.0);
				iwangle degAng = (iwangle)(IW_GEOM_ONE * perCentAngle);
				
				CIwVec2 center;
				center.x = (g_pConnectedImage->GetSize().x / 2);
				center.y = (g_pConnectedImage->GetSize().y / 2);

				CIwVec2 pos = g_pConnectedImage->GetPos();

				rotMat.SetRot(degAng, center);
				//g_pConnectedImage->SetTransform(rotMat);
			}

			char szConnecting[100];

			if (!g_pConnectingElem->IsVisible())
			{
				strcpy(szConnecting, "Connecting GPS...");

				g_pConnectingLabel->SetCaption(szConnecting);
				g_pConnectingElem->SetVisible(true);

				g_pStartButton->SetEnabled(false);
				g_pLoadMapButton->SetEnabled(false);
			}
		}
		else
		{
			if (g_pConnectingElem->IsVisible())
			{
				g_pConnectedImage->SetTransform(CIwUIMat::g_Identity);
				g_pLoadMapButton->SetEnabled(true);
				g_pConnectingElem->SetVisible(false);
				g_pStartButton->SetEnabled(true);
			}
		}
		if (0 == (g_tickCount % 300))
		{
			s3eLocation loc;
			s3eLocationGet(&loc);
		}
	}
	CIwColour c;
	c.r = c.g = c.b = 0xFF;

	int alpha = (g_tickCount*4) % 512;
	if (alpha > 255)
	{
		alpha = 512 - alpha;
	}
	if (alpha == 256)
	{
		alpha = 255;
	}
	c.a = alpha;
	g_pConnectedImage->SetColour(c);

	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);
}

void IntroGameState::PerformRender()
{
	IwGxSetScreenSpaceSlot(-1);
	
	CIwSVec2 backLoc(0,0);
	Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

	IwGxFlush();
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();
}

void IntroGameState::PerformActivate()
{
	// Force it to do an update immediately
	g_bWaitingOnGps = false;

	if (!g_pMapGame->IsGpsActive())
	{
		g_bWaitingOnGps = true;
	}
	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);
}

void IntroGameState::PerformDeActivate()
{
	//g_pDialogMain->SetVisible(false);
}

void IntroGameState::OnClickStartGame(CIwUIElement* Clicked)
{
 	if (!IwGetUIAnimManager()->IsAnimPlaying())
	{
		g_pMapGame->SetGameState(GPS_GameState_SelectGame, AnimDir_Right);
	}
}
void IntroGameState::OnClickLoadMap(CIwUIElement* Clicked)
{
}
void IntroGameState::OnClickHighScores(CIwUIElement* Clicked)
{
 	if (!IwGetUIAnimManager()->IsAnimPlaying())
	{
		g_pMapGame->SetGameState(GPS_GameState_HighScore, AnimDir_Right);
	}
}
void IntroGameState::OnClickHelp(CIwUIElement* Clicked)
{
 	if (!IwGetUIAnimManager()->IsAnimPlaying())
	{
		g_pMapGame->SetStaticContent("helpPanel");
		g_pMapGame->SetGameState(GPS_GameState_StaticContent, AnimDir_Right);
	}
}
void IntroGameState::OnClickExit(CIwUIElement* Clicked)
{
 	if (!IwGetUIAnimManager()->IsAnimPlaying())
	{
		g_pMapGame->Exit();
	}
}