#include "ActiveGameState.h"
#include "MessageBox.h"
#include "Constants.h"
#include "GoTheDistanceGameEngine.h"
#include "FindTheSpotGameEngine.h"
#include "BoomerangGameEngine.h"
#include "EggDropGameEngine.h"
#include "ShakeItGameEngine.h"
#include "CaptureGameEngine.h"

ActiveGameState::ActiveGameState(void)
{
	g_iAnimationCount = 0;
	g_cursorIter = 0;
	g_iStartGameCount = 0;

	IW_UI_CREATE_VIEW_SLOT1(this, "ActiveGameState", ActiveGameState, OnClickResume, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("active\\panel", "CIwUIElement");
	g_pDialogMain->SetVisible(false);
	g_pTitleBar = g_pDialogMain->GetChildNamed("TitleLabel", true);

	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	g_bIsScore = false;
	g_pFont = (CIwGxFont*)IwGetResManager()->GetResNamed("font_medium", "CIwGxFont");
	g_pFontLarge = (CIwGxFont*)IwGetResManager()->GetResNamed("font_large", "CIwGxFont");
	g_dAlpha = 100;
	g_bGameOver = false;
	g_pGameEngine = NULL;

	IwGetMultiplayerHandler()->RegisterModeChangedCallback(MultiplayerModeChanged, this);
	IwGetMultiplayerHandler()->RegisterDisconnectCallback(MultiplayerDisconnect, this);

	g_pScoreKeeper = new CScoreKeeper;
	g_pContentBlock = new CContentBlock;
	g_pTimer = new CCountdownTimer;
	g_barGraph = new CBarGraph;
	
	CIwRect bounds = g_pScoreKeeper->GetBounds();
	bounds.x = (Iw2DGetSurfaceWidth() - bounds.w) / 2;
	bounds.y = (Iw2DGetSurfaceHeight() - bounds.h - 10);
	g_pScoreKeeper->SetBounds(bounds);
	g_pScoreKeeper->SetValue(0);

	CIwRect tbounds = g_pTimer->GetBounds();
	tbounds.w = 110;
	tbounds.h = 110;
	tbounds.x = (Iw2DGetSurfaceWidth() - tbounds.w) / 2;
	tbounds.y = 10;
	g_pTimer->SetBounds(tbounds);
	g_pTimer->SetTotal(60000);

	CIwRect cbounds = g_pContentBlock->GetBounds();
	cbounds.x = (Iw2DGetSurfaceWidth() - 260) / 2;
	cbounds.w = 260;//(Iw2DGetSurfaceWidth() - 64);
	cbounds.y = tbounds.y + tbounds.h + 10;
	cbounds.h = 260;
	g_pContentBlock->SetBounds(cbounds);

	CIwRect scoreBounds(10, 50, Iw2DGetSurfaceWidth() - 20, Iw2DGetSurfaceHeight() - 100);
	g_barGraph->SetBounds(scoreBounds);

	g_pBackground = (CIwTexture*)IwGetResManager()->GetResNamed("fullback", "CIwTexture");
	g_pReady = (CIwTexture*)IwGetResManager()->GetResNamed("ready", "CIwTexture");
	g_pSet = (CIwTexture*)IwGetResManager()->GetResNamed("set", "CIwTexture");
	g_pGo = (CIwTexture*)IwGetResManager()->GetResNamed("go", "CIwTexture");

	g_games.push_back(new ShakeItGameEngine);
	g_games.push_back(new FindTheSpotGameEngine);
	g_games.push_back(new CaptureGameEngine);
	g_games.push_back(new GoTheDistanceGameEngine);
	g_games.push_back(new BoomerangGameEngine);
	g_games.push_back(new EggDropGameEngine);

	for (int i = 0; i < g_games.size(); ++i)
	{
		int value = 0;
		bool hasMatch = false;
		do
		{
			value = IwRandMinMax(0, g_games.size());

			hasMatch = false;
			for (int x = 0; x < g_gameIndices.size(); ++x)
			{
				if (g_gameIndices[x] == value)
				{
					hasMatch = true;
					break;
				}
			}
		} while (hasMatch);
		g_gameIndices.push_back(value);
	}

	g_gameState = PGS_INIT;

	g_iIntroCount = 300;
	g_iIntroReadyCount = 200;
	g_iIntroSetCount = 280;
}

ActiveGameState::~ActiveGameState(void)
{
	delete g_pFont;
	delete g_pBackground;
	delete g_pReady;
	delete g_pSet;

	delete g_barGraph;
	delete g_pContentBlock;
	delete g_pTimer;
	delete g_pScoreKeeper;

	for (int i = 0; i < g_games.size(); ++i)
	{
		g_games[i]->End();
		delete g_games[i];
	}

	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void ActiveGameState::MultiplayerDisconnect(bool success, const char* szStatus, void* userData)
{
	ActiveGameState* pThis = (ActiveGameState*)userData;

	MessageBox::Show("Disconnected", (char*)szStatus, "OK", "Cancel", NULL, userData);
	pThis->g_pMapGame->SetGameState(GPS_GameState_SelectGame, AnimDir_Right);
}

void ActiveGameState::MultiplayerModeChanged(MultiplayerMode mode, void* userData)
{
	if (mode == MPM_IN_GAME)
	{
		// We have started our game
		// Send our state & transition
		ActiveGameState* pThis = (ActiveGameState*)userData;
	}
}


void ActiveGameState::OnClickResume(CIwUIElement* Clicked)
{
	HideScore();
	StartNextGame();
}

void ActiveGameState::PerformActivate()
{
	for (int i = 0; i < g_games.size(); ++i)
	{
		g_games[i]->SetGameHandler(g_pMapGame);
	}
	g_barGraph->Reset();
	g_bGameOver = false;

	g_bRenderGame = false;

	g_bMouseDown = false;
	g_iStartGameCount = 0;
	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);

	g_bIsScore = false;
	g_pMapGame->StartTrackingDistance();

	g_mpLastFlush = 0;

	IwGetMultiplayerHandler()->Flush();
	g_mpLastFlush = s3eTimerGetMs();

	g_iGameIndex = -1;
	
	IwGetMultiplayerHandler()->RegisterCallback(0, ReceiveStatusUpdate, this);

	// We have our users & a game. If we are the master, we want to send out the game start

	g_iTotalScore = 0;
	StartNextGame();

	CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

	for (int i = 0; i < users.size(); ++i)
	{
		g_barGraph->AddBar(users[i]->szDevice, 0);
	}
}

void ActiveGameState::ReceiveStatusUpdate(const char * Result, uint32 ResultLen, void* userData)
{
	ActiveGameState* pThis = (ActiveGameState*)userData;
	PicnicGamesMessage* message = (PicnicGamesMessage*)Result;

	IwAssertMsg("FF", sizeof(PicnicGamesMessage) == ResultLen, ("sizeof(PicnicGamesMessage) != ResultLen (%d - %d)", sizeof(PicnicGamesMessage), ResultLen));

	PicnicGamesMessage networkMessage;
	memcpy(&networkMessage, Result, sizeof(PicnicGamesMessage));
	PrepareReceive_PicnicGamesMessage(&networkMessage, &networkMessage);

	switch (networkMessage.ev)
	{
		case PGE_GAME :
		{
			// TODO - we need to load the new game here & hide the score
			int gameIndex = networkMessage.dataA;
			pThis->g_iGameIndex = networkMessage.dataB;
			
			if (gameIndex < 0)
			{
				pThis->g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Left);
				pThis->HideScore();
			}
			else
			{
				pThis->g_pGameEngine = pThis->g_games[gameIndex];
				pThis->HideScore();

				pThis->g_gameState = PGS_INGAME;
			}
		}
		break;
		case PGE_SCORESINGLE :
		{
			if (IwGetMultiplayerHandler()->IsMaster())
			{
				pThis->SetUserScore(networkMessage.dataA, networkMessage.dataB, false);
			}
		}
		break;
		case PGE_SCOREMULTI :
		{
			if (!IwGetMultiplayerHandler()->IsMaster())
			{
				pThis->g_allUsersReady = true;
				CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();
				for (int i = 0; i < users.size(); ++i)
				{
					int16* pScoreFirst = &networkMessage.score1;
					int16* pScore = pScoreFirst + (sizeof(int16) * users[i]->uiIndex);

					users[i]->Score = *pScore;
					pThis->g_barGraph->AddBar(users[i]->szDevice, users[i]->Score);
				}
			}
		}
		break;
	}
}

void ActiveGameState::SetUserScore(int16 userId, int16 score, bool setMe)
{
	CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

	bool allUsersReady = true;
	for (int i = 0; i < users.size(); ++i)
	{
		if (users[i]->uiIndex == userId && (setMe || !users[i]->IsMe))
		{
			users[i]->Ready = true;
			users[i]->UserData = score;
		}
		if (!users[i]->Ready)
		{
			allUsersReady = false;
		}
	}

	// We need to force an update of the UI here to update the scores / graphs
	g_allUsersReady = allUsersReady;

	if (allUsersReady)
	{
		CIwArray<CIwMultiplayerHandler::User*> userList;

		for (int i = 0; i < users.size(); ++i)
		{
			uint insertPoint = userList.size();
			for (int j = 0; j < userList.size(); ++j)
			{
				if (users[i]->UserData < userList[j]->UserData)
				{
					insertPoint = j;
					break;
				}
			}
			userList.insert_slow(users[i], insertPoint);
		}

		// Sort the scores and sum
		int maxScore = users.size();
		int lastScore = INT16_MIN;
		for (int i = 0; i < userList.size(); ++i)
		{
			if (i > 0 && userList[i]->UserData != lastScore)
			{
				maxScore--;
			}
			userList[i]->Score += (maxScore * 10);
			lastScore = userList[i]->UserData;
		}
		PicnicGamesMessage message;
		message.ev = PGE_SCOREMULTI;

		for (int i = 0; i < users.size(); ++i)
		{
			int16* pScoreFirst = &message.score1;
			int16* pScore = pScoreFirst + (sizeof(int16) * users[i]->uiIndex);

			*pScore = users[i]->Score;
			int16 score = users[i]->Score;
			g_barGraph->AddBar(users[i]->szDevice, users[i]->Score);
		}

		if (IwGetMultiplayerHandler()->IsMultiplayer() && IwGetMultiplayerHandler()->IsMaster())
		{
			PrepareSend_PicnicGamesMessage(message, message);
			IwGetMultiplayerHandler()->Send(0, (char*)&message, sizeof(PicnicGamesMessage), true);
			IwGetMultiplayerHandler()->Flush();
		}
	}
}

void ActiveGameState::PerformDeActivate()
{
	g_pMapGame->StopTrackingDistance();
}

void ActiveGameState::StartNextGame()
{
	CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

	bool allUsersReady = true;
	for (int i = 0; i < users.size(); ++i)
	{
		users[i]->Ready = false;
	}

	if (IwGetMultiplayerHandler()->IsMaster())
	{
		g_iGameIndex++;
		int16 gameId = -1;

		if (g_iGameIndex < g_gameIndices.size())
		{
			g_pGameEngine = g_games[g_gameIndices[g_iGameIndex]];
			gameId = g_gameIndices[g_iGameIndex];
		}

		PicnicGamesMessage message;
		message.ev = PGE_GAME;
		message.dataA = gameId;
		message.dataB = g_iGameIndex;

		PrepareSend_PicnicGamesMessage(message, message);
		IwGetMultiplayerHandler()->Send(0, (char*)&message, sizeof(PicnicGamesMessage), true);
		IwGetMultiplayerHandler()->Flush();

		if (gameId >= 0)
		{
			g_gameState = PGS_INGAME;
		}
		else
		{
			this->GetGameHandler()->SetGameState(GPS_GameState_Intro, AnimDir_Left);
		}
	}
	else
	{
		g_gameState = PGS_WAITING;
	}
}

void ActiveGameState::PerformUpdate()
{
	s3eDeviceBacklightOn();
	// Update the background
	bool hasUpdate = false;
	bool renderUI = false;
	bool backClicked = false;
	bool becameScore = false;
	bool willFlush = false;

	if (g_gameState == PGS_WAITING)
	{
		// wait for the goahead
		// TODO - render the score & a waiting dialog

		renderUI = true;
		g_dAlpha = 75;

		g_barGraph->Update();

		CIwUIRect rect = g_pTitleBar->GetFrame();
		if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED) && (s3ePointerGetY() < rect.h))
		{
			backClicked = true;
		}
		else if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED)
		{
			if (IwGetMultiplayerHandler()->IsMaster())
			{
				// Hide the score & send the game to everyone else?
				HideScore();
				StartNextGame();
				//HideScore();
				//hasUpdate = true;
			}
		}
	}
	else if (g_gameState == PGS_INGAME)
	{
		bool isLoading = !(g_iStartGameCount > g_iIntroCount);

		if (g_iStartGameCount == 0)
		{
			g_pGameEngine->Prepare();
		}
		else if (!isLoading && !g_bRenderGame)
		{
			g_uiStartTime = s3eTimerGetMs();
			g_bRenderGame = true;
			g_pTimer->SetTotal(g_pGameEngine->GetGameLength());
			g_pGameEngine->Start();
		}

		Utils::UpdateLocation();
		
		int pulseInterval = (IwGetMultiplayerHandler()->IsMaster()) ? 500 : 500;
		if (IwGetMultiplayerHandler()->InSocketMode())
		{
			// TODO: increase interval if we are tilting
			pulseInterval = (IwGetMultiplayerHandler()->IsMaster()) ? 350 : 200;

			if (!becameScore && g_bIsScore)
			{
				pulseInterval = 2500;
			}
		}

		uint64 timer = s3eTimerGetMs();
		willFlush = ((timer - g_mpLastFlush) > pulseInterval);

		if (isLoading)
		{
			// show the 1 2 3 here
			g_iStartGameCount++;
		}
		else if (g_bIsScore)
		{
			renderUI = true;
			g_dAlpha = 75;

			CIwUIRect rect = g_pTitleBar->GetFrame();
			if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED) && (s3ePointerGetY() < rect.h))
			{
				backClicked = true;
			}
			else if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED)
			{
				if (IwGetMultiplayerHandler()->IsMaster())
				{
					// Hide the score & send the game to everyone else?
					//HideScore();
					//hasUpdate = true;
				}
			}
		}
		else if (g_bGameOver)
		{
			this->GetGameHandler()->SetGameState(GPS_GameState_Intro, AnimDir_Left);
		}
		else
		{
			CIwUIRect rect = g_pTitleBar->GetFrame();
			g_dAlpha = 150;
			if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED) && (s3ePointerGetY() < rect.h))
			{
				backClicked = true;
			}
			else
			{
				int32 gameLength = g_pGameEngine->GetGameLength();

				// Update the game engine here
				uint64 elapsedTime = (s3eTimerGetMs() - g_uiStartTime);
				uint64 remainingTime = gameLength - elapsedTime;
				g_pTimer->SetCurrent(elapsedTime);
				float fScore = 0;
				char szUnits[50];

				g_pGameEngine->Update(remainingTime, &fScore, szUnits);
				g_pTimer->Update();
				g_pContentBlock->Update();
				g_pScoreKeeper->SetValue(fScore);
				g_pScoreKeeper->SetUnits(szUnits);
				g_pScoreKeeper->Update();

				if (elapsedTime > gameLength)
				{
					g_gameState = PGS_WAITING;
					ShowScore();
					float finalScore = g_pGameEngine->End();
					g_iTotalScore += (int16)(finalScore * 100);

					if (!IwGetMultiplayerHandler()->IsMaster())
					{
						PicnicGamesMessage message;
						message.ev = PGE_SCORESINGLE;
						message.dataA = IwGetMultiplayerHandler()->GetMe()->uiIndex;
						message.dataB = (int16)(finalScore * 100);

						PrepareSend_PicnicGamesMessage(message, message);
						IwGetMultiplayerHandler()->Send(0, (char*)&message, sizeof(PicnicGamesMessage), true);
					}
					else
					{
						SetUserScore(IwGetMultiplayerHandler()->GetMe()->uiIndex, finalScore * 100, true);
					}

					hasUpdate = true;
				}
			}
		}
		// Always write level state
		if (willFlush || hasUpdate)
		{
			g_mpLastFlush = timer;
			IwGetMultiplayerHandler()->Flush();
		}
	}

	if (backClicked)
	{
		bool result = MessageBox::Show((char*)"Quit Game", (char*)"Are you sure you want to quit?", (char*)"Yes", (char*)"No", GameState::MessageRenderBackground, this);

		if (result)
		{
			if (g_pGameEngine)
			{
				g_pGameEngine->End();
			}
			IwGetMultiplayerHandler()->EndGame();
			g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Left);
		}
	}

	if (renderUI)
	{
		IwGetUIController()->Update();
		IwGetUIView()->Update(1000/20);
	}
}

void ActiveGameState::PerformRender()
{
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(-1);
	RenderBackground();

	if (g_gameState == PGS_WAITING)
	{
		// wait for the goahead
		// we should draw a waiting sign here
		// TODO - render the score & a waiting dialog
		CIwSVec2 backLoc(0,0);
		Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

		IwGxLightingOn();
		IwGxFontSetFont(g_pFontLarge);
		IwGxFontSetCol(0xff000000);
		IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
		IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);

		//Render the level
		Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
		Iw2DSetColour(g_IwGxColours[IW_GX_COLOUR_WHITE]);
		Iw2DFillRect(CIwSVec2(0, 0), CIwSVec2(Iw2DGetSurfaceWidth(), 40));

		CIwRect rectLevel(0, 0, Iw2DGetSurfaceWidth(), 40);
		IwGxFontSetRect(rectLevel);

		char szLevel[30];
		sprintf(szLevel, "Level %d of %d", g_iGameIndex+1, g_gameIndices.size());
		IwGxFontDrawText(szLevel);

		//Render the continue
		Iw2DSetColour(g_IwGxColours[IW_GX_COLOUR_WHITE]);
		Iw2DFillRect(CIwSVec2(0, Iw2DGetSurfaceHeight()-40), CIwSVec2(Iw2DGetSurfaceWidth(), 40));

		CIwRect rectScore(0, Iw2DGetSurfaceHeight()-40, Iw2DGetSurfaceWidth(), 40);

		if (!g_allUsersReady)
		{
			IwGxFontSetRect(rectScore);
			IwGxFontDrawText("Waiting for all user's scores...");
		}
		else
		{
			g_barGraph->Render();

			IwGxFontSetFont(g_pFont);
			IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
			IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
			IwGxFontSetRect(rectScore);
			if (IwGetMultiplayerHandler()->IsMaster())
			{
				IwGxFontDrawText("Tap the Screen to Continue...");
			}
			else
			{
				IwGxFontDrawText("Waiting for the next game...");
			}
		}
		IwGxLightingOff();
		IwGxFlush();
	}
	else if (g_gameState == PGS_INGAME)
	{
		// Render 2-d stuff here
		if (!g_bRenderGame)
		{
/*
			CIw2DImage* img;

			if (g_iStartGameCount < 20)
			{
				img = g3;
			}
			else if (g_iStartGameCount < 40)
			{
				img = g2;
			}
			else
			{
				img = g1;
			}
			CIwSVec2 pos;
			pos.x = (int16)((Iw2DGetSurfaceWidth() - img->GetWidth()) / 2);
			pos.y = (int16)((Iw2DGetSurfaceHeight() - img->GetHeight()) / 2);

			Iw2DSetColour(0xFFFFFFFF);
			IwGxSetColClear(0, 0, 0, 0);
			Iw2DDrawImage(img, pos);
*/
			CIwTexture* pTexture;
			if (g_iStartGameCount < g_iIntroReadyCount)
			{
				pTexture = g_pReady;
			}
			else if (g_iStartGameCount < g_iIntroSetCount)
			{
				pTexture = g_pSet;
			}
			else
			{
				pTexture = g_pGo;
			}

			// Render the game here
			CIwSVec2 backLoc(0,0);
			Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

			CIwSVec2 textLoc((Iw2DGetSurfaceWidth() - pTexture->GetWidth()) / 2,10);
			Utils::AlphaRenderImage(pTexture, textLoc, 255);

			CIwRect contentBounds = g_pContentBlock->GetInnerBounds();

			g_pContentBlock->Render();
			//g_pScoreKeeper->Render();
			//g_pTimer->Render();
			g_pGameEngine->RenderInfo(contentBounds);
		}
		else if (g_bIsScore)
		{
			IwGxFlush();
			IwGxClear(IW_GX_DEPTH_BUFFER_F);
			IwGxSetScreenSpaceSlot(0);

			CIwUIRect rect = g_pTitleBar->GetFrame();
			IwGetUIView()->Render();
		}
		else
		{
			// Render the game here
			CIwSVec2 backLoc(0,0);
			Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

			g_pScoreKeeper->Render();
			g_pTimer->Render();
			g_pContentBlock->Render();
			CIwRect contentBounds = g_pContentBlock->GetInnerBounds();
			g_pGameEngine->Render(contentBounds);
		}
	}
	Iw2DFinishDrawing();
}

void ActiveGameState::ShowScore()
{
	// TODO - only set it visible once everyone has reported their score, plus some time (5 seconds)
	g_bIsScore = true;
}

void ActiveGameState::HideScore()
{
	g_bIsScore = false;
	
	g_uiStartTime = s3eTimerGetMs();
	g_iStartGameCount = 0;
	g_bRenderGame = false;
}

void ActiveGameState::RenderBackground()
{
}
