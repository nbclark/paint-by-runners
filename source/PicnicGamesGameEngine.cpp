#include "PicnicGamesGameEngine.h"
#include "MessageBox.h"
#include "Utils.h"
#include <s3eVibra.h>

PicnicGamesGameEngine::PicnicGamesGameEngine()
{
}

void PicnicGamesGameEngine::Init(void* pGameStateVoid)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "PicnicGamesGameEngine", PicnicGamesGameEngine, OnClickResume, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("pause\\panel", "CIwUIElement");
	g_pResume = (CIwUIElement*)g_pDialogMain->GetChildNamed("Resume_Button", true, false);

	g_pDialogMain->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	IGameHandler* pGameState = (IGameHandler*)pGameStateVoid;
	g_pGameHandler = pGameState;

	g_pFontHuge = (CIwGxFont*)IwGetResManager()->GetResNamed("font_large", "CIwGxFont");
	g_pFont = (CIwGxFont*)IwGetResManager()->GetResNamed("font_medium", "CIwGxFont");
	g_pFontSmall = (CIwGxFont*)IwGetResManager()->GetResNamed("font_small", "CIwGxFont");
	
	CIwImage imgPicnicGames, imgPicnicGamesGlow, imgPicnicGamesStun, imgBolt, imgMine, imgBonusPicnicGames, imgBonusPicnicGamesGlow, imgLevel, imgLevel1, imgLevel2, imgLevel3, imgLevel4, imgLevel5, imgLevel6, imgLevel7;
	imgPicnicGames.LoadFromFile("images/fireflies/firefly.png");
	imgPicnicGamesGlow.LoadFromFile("images/fireflies/firefly_glow.png");
	imgPicnicGamesStun.LoadFromFile("images/fireflies/firefly_blue.png");
	imgBolt.LoadFromFile("images/fireflies/bolt.png");
	imgMine.LoadFromFile("images/fireflies/mine.png");
	imgBonusPicnicGames.LoadFromFile("images/fireflies/bonusPicnicGames.png");
	imgBonusPicnicGamesGlow.LoadFromFile("images/fireflies/bonusPicnicGames_glow.png");
	imgLevel.LoadFromFile("level.tga");
	imgLevel1.LoadFromFile("level1.png");
	imgLevel2.LoadFromFile("level2.png");
	imgLevel3.LoadFromFile("level3.png");
	imgLevel4.LoadFromFile("level4.png");
	imgLevel5.LoadFromFile("level5.png");
	imgLevel6.LoadFromFile("level6.png");
	imgLevel7.LoadFromFile("level7.png");

	g_pTexturePicnicGames = new CIwTexture();
	g_pTexturePicnicGames->CopyFromImage(&imgPicnicGames);
	g_pTexturePicnicGames->Upload();

	g_pTexturePicnicGamesGlow = new CIwTexture();
	g_pTexturePicnicGamesGlow->CopyFromImage(&imgPicnicGamesGlow);
	g_pTexturePicnicGamesGlow->Upload();

	g_pTexturePicnicGamesStun = new CIwTexture();
	g_pTexturePicnicGamesStun->CopyFromImage(&imgPicnicGamesStun);
	g_pTexturePicnicGamesStun->Upload();

	g_pTextureBolt = new CIwTexture();
	g_pTextureBolt->CopyFromImage(&imgBolt);
	g_pTextureBolt->Upload();

	g_pTextureMine = new CIwTexture();
	g_pTextureMine->CopyFromImage(&imgMine);
	g_pTextureMine->Upload();

	g_pTextureBonusPicnicGames = new CIwTexture();
	g_pTextureBonusPicnicGames->CopyFromImage(&imgBonusPicnicGames);
	g_pTextureBonusPicnicGames->Upload();

	g_pTextureBonusPicnicGamesGlow = new CIwTexture();
	g_pTextureBonusPicnicGamesGlow->CopyFromImage(&imgBonusPicnicGamesGlow);
	g_pTextureBonusPicnicGamesGlow->Upload();

	g_pTextureLevel = new CIwTexture();
	g_pTextureLevel->CopyFromImage(&imgLevel);
	g_pTextureLevel->Upload();

	g_pTextureLevel1 = new CIwTexture();
	g_pTextureLevel1->CopyFromImage(&imgLevel1);
	g_pTextureLevel1->Upload();

	g_pTextureLevel2 = new CIwTexture();
	g_pTextureLevel2->CopyFromImage(&imgLevel2);
	g_pTextureLevel2->Upload();

	g_pTextureLevel3 = new CIwTexture();
	g_pTextureLevel3->CopyFromImage(&imgLevel3);
	g_pTextureLevel3->Upload();

	g_pTextureLevel4 = new CIwTexture();
	g_pTextureLevel4->CopyFromImage(&imgLevel4);
	g_pTextureLevel4->Upload();

	g_pTextureLevel5 = new CIwTexture();
	g_pTextureLevel5->CopyFromImage(&imgLevel5);
	g_pTextureLevel5->Upload();

	g_pTextureLevel6 = new CIwTexture();
	g_pTextureLevel6->CopyFromImage(&imgLevel6);
	g_pTextureLevel6->Upload();

	g_pTextureLevel7 = new CIwTexture();
	g_pTextureLevel7->CopyFromImage(&imgLevel7);
	g_pTextureLevel7->Upload();

	for (uint32 i = 0; i < 20; ++i)
	{
		CPicnicGames* pObject = new CPicnicGames(pGameState, g_pTexturePicnicGames, g_pTexturePicnicGamesGlow, g_pTexturePicnicGamesStun);
		g_masterGameObjects.append(pObject);
		g_activeFireflies.push_back(pObject);
	}

	for (uint32 i = 0; i < 15; ++i)
	{
		CBonusPicnicGames* pObject = new CBonusPicnicGames(pGameState, g_pTextureBonusPicnicGames, g_pTextureBonusPicnicGamesGlow, g_pTexturePicnicGamesStun);
		g_masterGameObjects.append(pObject);
		g_activeBonusPicnicGamess.push_back(pObject);
	}

	for (uint32 i = 0; i < 15; ++i)
	{
		CMine* pObject = new CMine(pGameState, g_pTextureMine);
		g_masterGameObjects.append(pObject);
		g_activeMines.push_back(pObject);
	}
	g_pUser = NULL;
	g_pPulse = NULL;
}

PicnicGamesGameEngine::~PicnicGamesGameEngine(void)
{
	delete g_pTexturePicnicGames;
	delete g_pTexturePicnicGamesGlow;
	delete g_pTexturePicnicGamesStun;
	delete g_pTextureBolt;
	delete g_pTextureMine;
	delete g_pTextureBonusPicnicGames;
	delete g_pTextureBonusPicnicGamesGlow;
	delete g_pTextureLevel;
	delete g_pTextureLevel1;
	delete g_pTextureLevel2;
	delete g_pTextureLevel3;
	delete g_pTextureLevel4;
	delete g_pTextureLevel5;
	delete g_pTextureLevel6;
	delete g_pTextureLevel7;

	for (uint32 i = 0; i < g_activeFireflies.size(); ++i)
	{
		delete g_activeFireflies[i];
	}
	g_activeFireflies.clear();

	for (uint32 i = 0; i < g_activeBonusPicnicGamess.size(); ++i)
	{
		delete g_activeBonusPicnicGamess[i];
	}
	g_activeBonusPicnicGamess.clear();

	for (uint32 i = 0; i < g_activeMines.size(); ++i)
	{
		delete g_activeMines[i];
	}
	g_activeMines.clear();

	DeActivate();
}

void PicnicGamesGameEngine::ResetObjects()
{
	CIwFVec2 metersPerSecond(1, 1);
	float glowCycleSecondDuration = g_iPicnicGamesGlowDuration;
	metersPerSecond.x = g_fPicnicGamesSpeed.x;
	metersPerSecond.y = g_fPicnicGamesSpeed.y;
	
	for (uint32 i = 0; i < g_activeFireflies.size(); ++i)
	{
		g_activeFireflies[i]->SetTopBound(g_bounds.y);
		g_activeFireflies[i]->SetActive(true);
		g_activeFireflies[i]->SetDifficulty(metersPerSecond, glowCycleSecondDuration);
	}

	CIwFVec2 metersPerSecondD(metersPerSecond.x * 3, metersPerSecond.y * 3);
	for (uint32 i = 0; i < g_activeBonusPicnicGamess.size(); ++i)
	{
		g_activeBonusPicnicGamess[i]->SetTopBound(g_bounds.y);
		g_activeBonusPicnicGamess[i]->SetActive(true);
		g_activeBonusPicnicGamess[i]->SetDifficulty(metersPerSecond, glowCycleSecondDuration / 2);
	}
	
	for (uint32 i = 0; i < g_activeMines.size(); ++i)
	{
		g_activeMines[i]->SetTopBound(g_bounds.y);
		g_activeMines[i]->SetActive(true);
		g_activeMines[i]->Reset();
	}

	g_pUser->Reset();
}

void PicnicGamesGameEngine::Activate(CIwRect bounds)
{
	g_bounds = bounds;
	DeActivate();

	g_iLevel = 0;
	g_iPulseCount = 2;
	g_iScore = g_iDisplayedScore = 0;
	g_handler.Start(1500, 3, 1000);

	// We need to wire up all of our objects here, I think
	for (uint32 i = 0; i < g_masterGameObjects.size(); ++i)
	{
		((IGameObject*)g_masterGameObjects[i])->ResetMultiplayer();
	}

	IwGetMultiplayerHandler()->RegisterCallback(0, ReceiveStatusUpdate, this);

	// Add multiplayer-users
	if (IwGetMultiplayerHandler()->IsMultiplayer())
	{
		CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

		for (uint32 uu = 0; uu < users.size(); ++uu)
		{
			CGPSUser* pUser =  new CGPSUser(g_pGameHandler, users[uu], users[uu]->IsMe ? (char*)"images/fireflies/user.png" : (char*)"images/fireflies/userB.png", users[uu]->IsMe);
			CPulse* pPulse = new CPulse(g_pGameHandler, pUser, users[uu]->IsMe);

			if (users[uu]->IsMe)
			{
				g_pUser = pUser;
				g_pPulse = pPulse;
			}

			g_users.append(pUser);
			g_pulses.append(pPulse);

			g_userObjects.append(pUser);

			// Send all the user data and pulses
			g_masterGameObjects.append(pUser);
			g_masterGameObjects.append(pPulse);

			pUser->ResetMultiplayer();
			pPulse->ResetMultiplayer();
		}
	}
	else
	{
		g_pUser = new CGPSUser(g_pGameHandler, NULL, (char*)"images/fireflies/user.png", true);
		g_pPulse = new CPulse(g_pGameHandler, g_pUser, true);

		g_users.append(g_pUser);
		g_pulses.append(g_pPulse);
	}

	SetLevel();
}

void PicnicGamesGameEngine::DeActivate()
{
	for (uint32 i = 0; i < g_users.size(); ++i)
	{
		g_masterGameObjects.find_and_remove(g_users[i]);
		delete g_users[i];
	}
	g_users.clear();

	for (uint32 i = 0; i < g_pulses.size(); ++i)
	{
		g_masterGameObjects.find_and_remove(g_pulses[i]);
		delete g_pulses[i];
	}
	g_pulses.clear();
	g_handler.Stop();

	g_pUser = NULL;
	g_pPulse = NULL;
}

bool PicnicGamesGameEngine::UpdateLevel()
{
	g_iPulseCount++;
	g_iLevel = g_iLevel + 1;

	if (g_iLevel > 6)
	{
		// Signal game over
		return true;
	}

	SetLevel();
	return false;
}

int PicnicGamesGameEngine::GetScore()
{
	return g_pUser->GetScore();
}

void PicnicGamesGameEngine::SetLevel()
{
	g_bLevelOver = false;
	g_bGameOver = false;
	g_bRenderBonusPicnicGames = false;
	g_bRenderMine = false;
	g_iPicnicGamesPoints = 100;
	g_iMinePoints = 800;
	g_iPicnicGamesGlowDuration = 20;
	g_fPicnicGamesSpeed.x = 5;
	g_fPicnicGamesSpeed.y = 5;
	g_iMineCount = g_iBonusPicnicGamesCount = 0;
	g_iStunDuration = 10;
	
	g_iBonusPicnicGamesCount = g_iLevel + 1;
	g_bRenderBonusPicnicGames = true;

	g_iMineCount = g_iLevel + 1;
	g_bRenderMine = true;

	switch (g_iLevel)
	{
		case 1 :
			{
				g_iPicnicGamesPoints = 200;
				g_iBonusPicnicGamesCount = 0;
			}
			break;
		case 2 :
			{
				g_iPicnicGamesPoints = 300;
				g_bRenderBonusPicnicGames = true;
			}
			break;
		case 3 :
			{
				g_iPicnicGamesPoints = 400;
				g_bRenderBonusPicnicGames = true;
				g_bRenderMine = true;
				g_iMineCount = 5;
			}
			break;
		case 4 :
			{
				g_iPicnicGamesPoints = 600;
				g_bRenderBonusPicnicGames = true;
				g_bRenderMine = true;

				g_iPicnicGamesGlowDuration = 15;
				g_fPicnicGamesSpeed.x = g_fPicnicGamesSpeed.y = 7;
				g_iMineCount = 5;
			}
			break;
		case 5 :
			{
				g_iPicnicGamesPoints = 800;
				g_bRenderBonusPicnicGames = true;
				g_bRenderMine = true;

				g_iPicnicGamesGlowDuration = 10;
				g_fPicnicGamesSpeed.x = g_fPicnicGamesSpeed.y = 10;
				g_iMineCount = 10;
			}
			break;
		case 6 :
			{
				g_iPicnicGamesPoints = 1000;
				g_bRenderBonusPicnicGames = true;
				g_bRenderMine = true;

				g_iPicnicGamesGlowDuration = 5;
				g_fPicnicGamesSpeed.x = g_fPicnicGamesSpeed.y = 15;
				g_iMineCount = 15;
			}
			break;
		default :
			{
			}
			break;
	}
	g_iMinePoints = g_iPicnicGamesPoints * 2;

	ResetObjects();

	// set the # of bonusPicnicGamesflies
	for (uint32 i = 0; i < g_activeBonusPicnicGamess.size(); ++i)
	{
		if (i >= g_iBonusPicnicGamesCount)
		{
			g_activeBonusPicnicGamess[i]->SetActive(false);
		}
		else
		{
			g_activeBonusPicnicGamess[i]->SetActive(true);
		}
	}

	// set the # of mines
	for (uint32 i = 0; i < g_activeMines.size(); ++i)
	{
		if (i >= g_iMineCount)
		{
			g_activeMines[i]->SetActive(false);
		}
		else
		{
			g_activeMines[i]->SetActive(true);
		}
	}
	for (uint32 i = 0; i < g_activeFireflies.size(); ++i)
	{
		g_activeFireflies[i]->SetActive(true);
	}
}

void PicnicGamesGameEngine::WriteLevelState()
{
	if (IwGetMultiplayerHandler()->IsMaster())
	{
		PicnicGamesMessage collisionMessage;
		collisionMessage.ev = FFM_LEVEL;
		collisionMessage.dataA = g_iLevel;
		collisionMessage.dataB = g_pHost->IsScore();

		PrepareSend_PicnicGamesMessage(collisionMessage, collisionMessage);
		IwGetMultiplayerHandler()->Send(0, (char*)&collisionMessage, sizeof(PicnicGamesMessage), false);
	}
	else
	{
		// Write our position -- this might just be handled by our parent
		g_pUser->SendStatusUpdate(true);
	}
}

void PicnicGamesGameEngine::WriteGameState()
{
	// When we create our objects, they register for status updates
	// Send out a status update here
	if (IwGetMultiplayerHandler()->IsMaster())
	{
		// Send out a status update
		// we want to send all of our objects
		for (uint32 i = 0; i < g_masterGameObjects.size(); ++i)
		{
			((IGameObject*)g_masterGameObjects[i])->SendStatusUpdate(true);
		}
	}
	else
	{
		g_pPulse->SendStatusUpdate(true);
	}
}

void PicnicGamesGameEngine::ReceiveStatusUpdate(const char * Result, uint32 ResultLen, void* userData)
{
	PicnicGamesGameEngine* pThis = (PicnicGamesGameEngine*)userData;
	PicnicGamesMessage* message = (PicnicGamesMessage*)Result;

	IwAssertMsg("FF", sizeof(PicnicGamesMessage) == ResultLen, ("sizeof(PicnicGamesMessage) != ResultLen (%d - %d)", sizeof(PicnicGamesMessage), ResultLen)); 

	PicnicGamesMessage networkMessage;
	memcpy(&networkMessage, Result, sizeof(PicnicGamesMessage));
	PrepareReceive_PicnicGamesMessage(&networkMessage, &networkMessage);

	switch (networkMessage.ev)
	{
		case FFM_LEVEL :
			{
				// Force the level here
				if (networkMessage.dataA != pThis->g_iLevel)
				{
					pThis->g_iLevel = networkMessage.dataA;
					pThis->SetLevel();
				}

				bool showScore = (networkMessage.dataB != 0);
				if (showScore != pThis->g_pHost->IsScore())
				{
					if (showScore)
					{
						pThis->g_pHost->ShowScore();
					}
					else
					{
						pThis->g_bUnpause = true;
					}
				}
			}
			break;
		case FFM_COLLISION :
			{
				for (uint32 ff = 0; ff < pThis->g_activeFireflies.size(); ++ff)
				{
					if (networkMessage.dataA == pThis->g_activeFireflies[ff]->GetIdentifier())
					{
						if (pThis->g_activeFireflies[ff]->IsActive())
						{
							pThis->g_activeFireflies[ff]->SetActive(false);

							if (networkMessage.dataB == pThis->g_pUser->GetIdentifier())
							{
								pThis->g_activeFireflies[ff]->MakeCapture(pThis->g_locJar);
								int score = pThis->g_pUser->GetScore() + networkMessage.dataC;
								pThis->g_pUser->SetScore(score);
								break;
							}
						}
						break;
					}
				}
				for (uint32 ff = 0; ff < pThis->g_activeBonusPicnicGamess.size(); ++ff)
				{
					if (networkMessage.dataA == pThis->g_activeBonusPicnicGamess[ff]->GetIdentifier())
					{
						if (pThis->g_activeBonusPicnicGamess[ff]->IsActive())
						{
							pThis->g_activeBonusPicnicGamess[ff]->SetActive(false);

							if (networkMessage.dataB == pThis->g_pUser->GetIdentifier())
							{
								pThis->g_activeBonusPicnicGamess[ff]->MakeCapture(pThis->g_locJar);
								int score = pThis->g_pUser->GetScore() + networkMessage.dataC;
								pThis->g_pUser->SetScore(score);
								break;
							}
						}
						break;
					}
				}
				for (uint32 mm = 0; mm < pThis->g_activeMines.size(); ++mm)
				{
					if (networkMessage.dataA == pThis->g_activeMines[mm]->GetIdentifier())
					{
						if (pThis->g_activeMines[mm]->IsActive())
						{
							pThis->g_activeMines[mm]->SetActive(false);
							if (networkMessage.dataB == pThis->g_pUser->GetIdentifier())
							{
								pThis->g_activeMines[mm]->MakeCapture(pThis->g_locJar);
								int score = pThis->g_pUser->GetScore() + networkMessage.dataC;
								pThis->g_pUser->SetScore(score);
								break;
							}
						}

						break;
					}
				}
			}
			break;
		case FFM_LEVEL_OVER :
			{
				// set the level flag and handle in update
				pThis->g_bLevelOver = true;
			}
			break;
		case FFM_GAME_OVER :
			{
				// set the game over flag and handle in update
				pThis->g_bGameOver = true;
			}
			break;
	}
}

bool PicnicGamesGameEngine::Update(uint64 uiGameTimer, bool* pbVictory, bool* pbHasUpdate)
{
	bool hasUpdate = false;
	if (g_bLevelOver)
	{
		// Force the score to the actual value
		g_iDisplayedScore = g_iScore;

		*pbVictory = true;
		return true;
	}
	if (g_bGameOver)
	{
		s3eDeviceYield(0);
	}

	g_pUser->Update(pbVictory);

	if (s3ePointerGetState(S3E_POINTER_BUTTON_RIGHTMOUSE) & S3E_POINTER_STATE_RELEASED)
	{
		if (g_iPulseCount > 0)
		{
			g_iPulseCount--;
			g_pPulse->SetPulse(200, 0.25);
		}
	}

	bool isPulseActive = g_pPulse->IsActive();
	if (g_iPulseCount > 0 && !isPulseActive)
	{
		g_handler.Update();
		float magnitude = g_handler.GetShakeMagnitude();

		if (magnitude != 0)
		{
			g_iPulseCount--;
			g_pPulse->SetPulse((int)(75 * magnitude), 0.25);

			g_handler.Reset();
			s3eVibraVibrate(150, 250);

			hasUpdate = true;
		}
	}

	for (uint32 i = 0; i < g_users.size(); ++i)
	{
		g_users[i]->Update(pbVictory);
	}

	for (uint32 i = 0; i < g_pulses.size(); ++i)
	{
		if (g_pulses[i]->ShouldRender())
		{
			g_pulses[i]->Update(pbVictory);
		}
	}
	
	// Remove inactive objects
	for (uint32 i = 0; i < g_activeFireflies.size(); ++i)
	{
		if (g_activeFireflies[i]->ShouldRender())
		{
			g_activeFireflies[i]->Update(pbVictory);
		}
	}
	
	if (g_bRenderMine)
	{
		for (uint32 i = 0; i < g_activeMines.size(); ++i)
		{
			if (g_activeMines[i]->ShouldRender())
			{
				g_activeMines[i]->Update(pbVictory);
			}
		}
	}
	if (g_bRenderBonusPicnicGames)
	{
		for (uint32 i = 0; i < g_activeBonusPicnicGamess.size(); ++i)
		{
			if (g_activeBonusPicnicGamess[i]->ShouldRender())
			{
				g_activeBonusPicnicGamess[i]->Update(pbVictory);
			}
		}
	}

	// All collision detection happens on the master
	//if (IwGetMultiplayerHandler()->IsMaster())
	{
		if (g_bRenderMine)
		{
			for (uint32 i = 0; i < g_activeMines.size(); ++i)
			{
				if (g_activeMines[i]->IsActive())
				{
					for (uint32 uu = 0; uu < g_users.size(); ++uu)
					{
						if (g_users[uu]->InsersectsWith(g_activeMines[i]))
						{
							g_activeMines[i]->SetActive(false);
							g_users[uu]->SetScore(MAX(g_users[uu]->GetScore()-g_iMinePoints, 0));

							// nclark - send a collision if we aren't game master but user owner
							if (IwGetMultiplayerHandler()->IsMaster() || g_users[uu]->IsMaster())
							{
								PicnicGamesMessage collisionMessage;
								collisionMessage.ev = FFM_COLLISION;
								collisionMessage.dataA = g_activeMines[i]->GetIdentifier();
								collisionMessage.dataB = g_users[uu]->GetIdentifier();
								collisionMessage.dataC = g_iMinePoints;

								hasUpdate = true;
								PrepareSend_PicnicGamesMessage(collisionMessage, collisionMessage);
								IwGetMultiplayerHandler()->Send(0, (char*)&collisionMessage, sizeof(PicnicGamesMessage), true);
							}

							// TODO - move this elsewhere
							if (g_users[uu]->IsMaster())
							{
								g_activeMines[i]->MakeCapture(g_locJar);
								// if this is us -- vibrate
								s3eVibraVibrate(255, 500);
							}
							break;
						}
					}
				}
			}
		}

		for (uint32 uu = 0; uu < g_users.size(); ++uu)
		{
			if (g_bRenderBonusPicnicGames)
			{
				for (uint32 df = 0; df < g_activeBonusPicnicGamess.size(); ++df)
				{
					CBonusPicnicGames* pBonusPicnicGames = g_activeBonusPicnicGamess[df];

					if (pBonusPicnicGames->IsActive() && pBonusPicnicGames->IsVisible() && pBonusPicnicGames->InsersectsWith(g_users[uu]))
					{
						pBonusPicnicGames->SetActive(false);

						uint64 points = (g_iPicnicGamesPoints * 4) - ( (uiGameTimer / 1000) / 5 );

						int score = g_users[uu]->GetScore() + (int)(MIN((g_iPicnicGamesPoints * 4), MAX(0, points)));
						g_users[uu]->SetScore(score);

						if (IwGetMultiplayerHandler()->IsMaster() || g_users[uu]->IsMaster())
						{
							PicnicGamesMessage collisionMessage;
							collisionMessage.ev = FFM_COLLISION;
							collisionMessage.dataA = g_users[uu]->GetIdentifier();
							collisionMessage.dataB = pBonusPicnicGames->GetIdentifier();
							collisionMessage.dataC = points;

							hasUpdate = true;
							PrepareSend_PicnicGamesMessage(collisionMessage, collisionMessage);
							IwGetMultiplayerHandler()->Send(0, (char*)&collisionMessage, sizeof(PicnicGamesMessage), true);
						}
						if (g_users[uu]->IsMaster())
						{
							pBonusPicnicGames->MakeCapture(g_locJar);
							s3eVibraVibrate(100, 100);
						}
					}
				}
			}
			for (uint32 ff = 0; ff < g_activeFireflies.size(); ++ff)
			{
				CPicnicGames* pPicnicGames = g_activeFireflies[ff];

				if (pPicnicGames->IsActive() && pPicnicGames->IsVisible())
				{
					if (pPicnicGames->InsersectsWith(g_users[uu]))
					{
						pPicnicGames->SetActive(false);

						// Vary this on time taken
						// Get the total seconds
						uint64 points = g_iPicnicGamesPoints - ( (uiGameTimer / 1000) / 5 );

						int score = g_users[uu]->GetScore() + (int)(MIN(g_iPicnicGamesPoints, MAX(0, points)));
						g_users[uu]->SetScore(score);

						// nclark - send a collision if we aren't game master but user owner
						if (IwGetMultiplayerHandler()->IsMaster() || g_users[uu]->IsMaster())
						{
							PicnicGamesMessage collisionMessage;
							collisionMessage.ev = FFM_COLLISION;
							collisionMessage.dataA = pPicnicGames->GetIdentifier();
							collisionMessage.dataB = g_users[uu]->GetIdentifier();
							collisionMessage.dataC = points;

							hasUpdate = true;
							PrepareSend_PicnicGamesMessage(collisionMessage, collisionMessage);
							IwGetMultiplayerHandler()->Send(0, (char*)&collisionMessage, sizeof(PicnicGamesMessage), true);
						}

						if (g_users[uu]->IsMaster())
						{
							pPicnicGames->MakeCapture(g_locJar);
							s3eVibraVibrate(100, 100);
						}
					}
					else
					{
						bool wasHandled = false;

						if (!wasHandled)
						{
							for (uint32 pp = 0; pp< g_pulses.size(); ++pp)
							{
								if (g_pulses[pp]->IsActive() && g_pulses[pp]->InsersectsWith(pPicnicGames))
								{
									wasHandled = true;
									pPicnicGames->Stun(g_iStunDuration);
								}
							}
						}
					}
				}
			}
		}
	}
	
	g_iScore = g_pUser->GetScore();

	if (g_iDisplayedScore < g_iScore)
	{
		g_iDisplayedScore = MIN(g_iScore, g_iDisplayedScore + 10);
	}
	else if (g_iDisplayedScore > g_iScore)
	{
		g_iDisplayedScore = MAX(g_iScore, g_iDisplayedScore - 10);
	}

	int activeFireflies = 0;
	for (uint32 ff = 0; ff < g_activeFireflies.size(); ++ff)
	{
		if (g_activeFireflies[ff]->IsActive())
		{
			activeFireflies++;
		}
	}

	// If we are the master and are out of fireflies, end the level
	if (IwGetMultiplayerHandler()->IsMaster() && activeFireflies == 0)
	{
		// nclark - might need this
		// we removed it because we need to end the level *after* we get the score
		//if (IwGetMultiplayerHandler()->IsMultiplayer() && IwGetMultiplayerHandler()->IsMaster())
		//{
		//	PicnicGamesMessage collisionMessage;
		//	collisionMessage.ev = FFM_LEVEL_OVER;

		//	hasUpdate = true;
		//	PrepareSend_PicnicGamesMessage(collisionMessage, collisionMessage);
		//	IwGetMultiplayerHandler()->Send(0, (char*)&collisionMessage, sizeof(PicnicGamesMessage), true);
		//}
		*pbVictory = true;

		// Force the score to the actual value
		g_iDisplayedScore = g_iScore;
		return true;
	}

	if (hasUpdate)
	{
		*pbHasUpdate = true;
	}

	return false;
}

void PicnicGamesGameEngine::RenderStatus(CIwUIRect& bounds)
{
	IwGxLightingOn();
	IwGxFontSetFont(g_pFontHuge);
	IwGxFontSetCol(0xffffffff);
	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_LEFT);

	CIwRect rect((int16)(bounds.x+5), (int16)bounds.y, (int16)(bounds.w-5), (int16)bounds.h);
	IwGxFontSetRect(rect);
	sprintf(g_szStatus, "%06d", g_iDisplayedScore);
	IwGxFontDrawText(g_szStatus);
	
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
	IwGxFontSetFont(g_pFontSmall);
	
	int activeFireflies = 0;
	for (uint32 ff = 0; ff < g_activeFireflies.size(); ++ff)
	{
		if (g_activeFireflies[ff]->IsActive())
		{
			activeFireflies++;
		}
	}

	rect.w = 32;
	rect.x = (int16)(bounds.w - 32);
	IwGxFontSetRect(rect);
	sprintf(g_szStatus, "%02d", activeFireflies);
	IwGxFontDrawText(g_szStatus);

	rect.x = (int16)(bounds.w - 96);
	IwGxFontSetRect(rect);
	sprintf(g_szStatus, "%02d", g_iPulseCount);
	IwGxFontDrawText(g_szStatus);

	IwGxLightingOff();

	CIwSVec2 locFly((int16)(bounds.w - 64), (int16)((bounds.h - g_pTexturePicnicGamesGlow->GetWidth()) / 2));
	Utils::AlphaRenderImage(g_pTexturePicnicGamesGlow, locFly, 255);
	
	g_locJar = locFly;

	CIwSVec2 locBolt((int16)(bounds.w - 64 - 64), (int16)((bounds.h - g_pTextureBolt->GetWidth()) / 2));
	Utils::AlphaRenderImage(g_pTextureBolt, locBolt, 255);
}

void PicnicGamesGameEngine::RenderGX()
{
	//s3eDebugTraceLine("PicnicGamesGameEngine:RenderGX");

	CIwMat viewMat = IwGxGetViewMatrix();
	CIwMat fullMat;
	fullMat.SetIdentity();
	fullMat.t.z = viewMat.t.z;
	IwGxSetViewMatrix(&fullMat);

	for (uint32 i = 0; i < g_activeFireflies.size(); ++i)
	{
		if (g_activeFireflies[i]->ShouldRender())
		{
			g_activeFireflies[i]->Render();
		}
	}

	for (uint32 i = 0; i < g_activeBonusPicnicGamess.size(); ++i)
	{
		if (g_activeBonusPicnicGamess[i]->ShouldRender())
		{
			g_activeBonusPicnicGamess[i]->Render();
		}
	}

	for (uint32 i = 0; i < g_activeMines.size(); ++i)
	{
		if (g_activeMines[i]->ShouldRender())
		{
			g_activeMines[i]->Render();
		}
	}

	IwGxSetViewMatrix(&viewMat);

	for (uint32 i = 0; i < g_users.size(); ++i)
	{
		g_users[i]->Render();
	}
}

void PicnicGamesGameEngine::Render2D()
{
	for (uint32 i = 0; i < g_pulses.size(); ++i)
	{
		if (g_pulses[i]->IsActive())
		{
			g_pulses[i]->Render();
		}
	}
}

void PicnicGamesGameEngine::OnClickResume(CIwUIElement* Clicked)
{
	g_pDialogMain->SetVisible(false);
	g_bUnpause = true;

	// Force a double-read here, so we don't go into pause again right after.
	s3ePointerUpdate();
	s3ePointerUpdate();
}

void PicnicGamesGameEngine::ShowPause(bool bVisible)
{
	g_bUnpause = !bVisible;
	g_pDialogMain->SetVisible(bVisible);
	g_pResume->SetVisible(true);

	ShowScoreAndPause(bVisible);
}

void PicnicGamesGameEngine::ShowScore(bool bVisible)
{
	g_bUnpause = !bVisible;
	g_pDialogMain->SetVisible(bVisible);
	g_pResume->SetVisible(IwGetMultiplayerHandler()->IsMaster());

	ShowScoreAndPause(bVisible);
}

void PicnicGamesGameEngine::ShowScoreAndPause(bool bVisible)
{
	if (bVisible)
	{
		static CIwMultiplayerHandler::User me;
		me.Accepted = true;
		strcpy(me.szDevice, "Me");
		strcpy(me.szName, "Me");
		me.Score = g_iScore;
		me.IsMe = true;

		g_bUnpause = false;
		
		CIwUIElement* pItems = g_pDialogMain->GetChildNamed("MiddleLayer")->GetChild(0)->GetChildNamed("HelpUsers", true, true);
		CIwUIElement* pButtonTemplate = (CIwUIElement*)IwGetResManager()->GetResNamed("UserScoreTemplate", "CIwUIElement");
		
		CIwArray<CIwMultiplayerHandler::User*> pMPUsers = IwGetMultiplayerHandler()->ListUsers();

		for (int i = 0; i < pItems->GetNumChildren(); ++i)
		{
			CIwUIElement* pElem = pItems->GetChild(i);
			pItems->RemoveChild(pElem);

			delete pElem;
			i--;
		}

		CIwArray<CIwMultiplayerHandler::User*> pUsers;
		if (pUsers.size() == 0)
		{
			pUsers.append(&me);
		}
		else
		{
			for (uint32 i = 0; i < pMPUsers.size(); ++i)
			{
				pUsers.append(pMPUsers[i]);
			}
		}
		for (uint32 i = 0; i < pUsers.size(); ++i)
		{
			CIwUIElement* pTemplateNew = pButtonTemplate->Clone();
			CIwUIButton* pButtonNew = (CIwUIButton*)pTemplateNew->GetChildNamed("ButtonBase");
			CIwUIImage* pImage = (CIwUIImage*)pButtonNew->GetChildNamed("ButtonTemplate_Image", true, false);
			CIwUILabel* pLabelText = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Text", true, false);
			CIwUILabel* pLabelDistance = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Status", true, false);

			const char* szDevice = pUsers[i]->szDevice;
			const char* szName = pUsers[i]->szName;
			
			pTemplateNew->SetName(szDevice);
			pLabelText->SetCaption(szName);

			char szScore[10];
			sprintf(szScore, "%06d", pUsers[i]->Score);
			pLabelDistance->SetCaption(szScore);

			if (pUsers[i]->IsMe)
			{
				CIwTexture* x = (CIwTexture*)IwGetResManager()->GetResNamed("user", "CIwTexture");
				pImage->SetTexture((CIwTexture*)IwGetResManager()->GetResNamed("user", "CIwTexture"));
			}
			else
			{
				pImage->SetTexture((CIwTexture*)IwGetResManager()->GetResNamed("userB", "CIwTexture"));
			}
			
			pItems->GetLayout()->AddElement(pTemplateNew);
		}
	}
}

bool PicnicGamesGameEngine::RenderPause()
{
	return RenderScoreAndPause(false);
}

bool PicnicGamesGameEngine::RenderScore()
{
	return RenderScoreAndPause(true);
}

bool PicnicGamesGameEngine::RenderScoreAndPause(bool isScore)
{
	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);
	
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();

	return g_bUnpause;

	/*
	int width = Iw2DGetSurfaceWidth();
	int height = Iw2DGetSurfaceHeight();

	int iconWidth = 40;
	int iconHeight = 40;

	CIwRect bounds(15, height - 170, width - 30, iconHeight);

	IwGxLightingOn();
	IwGxFontSetFont(g_pFont);
	IwGxFontSetCol(0xffffffff);

	CIwRect rect(0, 0, width, bounds.y - g_pTextureLevel->GetHeight());
	IwGxFontSetRect(rect);
	IwGxFontAlignHor alignH = IwGxFontGetAlignmentHor();
	IwGxFontAlignVer alignV = IwGxFontGetAlignmentVer();

	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
	IwGxFontDrawText("Tap the screen to resume...");
	
	if (IwGetMultiplayerHandler()->IsMultiplayer())
	{
		// Show a list of all the users & scores here. Use the help UI perhaps?
		CIwArray<CIwMultiplayerHandler::User*> users = IwGetMultiplayerHandler()->ListUsers();

		for (uint32 i = 0; i < users.size(); ++i)
		{
		}
	}

	CIwTexture* pCurrLevel = g_pTextureLevel1;

	switch (g_iLevel)
	{
		case 0 :
		{
			pCurrLevel = g_pTextureLevel1;
		}
		break;
		case 1 :
		{
			pCurrLevel = g_pTextureLevel2;
		}
		break;
		case 2 :
		{
			pCurrLevel = g_pTextureLevel3;
		}
		break;
		case 3 :
		{
			pCurrLevel = g_pTextureLevel4;
		}
		break;
		case 4 :
		{
			pCurrLevel = g_pTextureLevel5;
		}
		break;
		case 5 :
		{
			pCurrLevel = g_pTextureLevel6;
		}
		break;
		case 6 :
		{
			pCurrLevel = g_pTextureLevel7;
		}
		break;
	}


	int totalWidth = g_pTextureLevel->GetWidth() + pCurrLevel->GetWidth() + 10;
	int totalHeight = g_pTextureLevel->GetHeight();

	CIwSVec2 locLevel((width - totalWidth) / 2, rect.h - totalHeight);
	Utils::AlphaRenderImage(g_pTextureLevel, locLevel, 255);

	locLevel.x += (int16)(g_pTextureLevel->GetWidth() + 10);
	Utils::AlphaRenderImage(pCurrLevel, locLevel, 255);

	Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
	Iw2DFillRect(CIwSVec2(10, height - 170), CIwSVec2(width - 20, 160));
	Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);

	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_LEFT);
	IwGxFontSetFont(g_pFontSmall);

	CIwRect fontBounds(15 + iconWidth + 10, height - 170, width - 40 - iconWidth, iconHeight);
	CIwSVec2 loc1(bounds.x + (iconWidth - g_pTexturePicnicGames->GetWidth()) / 2, bounds.y + (iconHeight - g_pTexturePicnicGames->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTexturePicnicGames, loc1, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Capture firelfies to gain points");

	bounds.y += 40;
	fontBounds.y += 40;
	CIwSVec2 loc2(bounds.x + (iconWidth - g_pTextureBonusPicnicGames->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureBonusPicnicGames->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureBonusPicnicGames, loc2, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("BonusPicnicGamesflies capture fireflies");

	bounds.y += 40;
	fontBounds.y += 40;
	CIwSVec2 loc3(bounds.x + (iconWidth - g_pTextureMine->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureMine->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureMine, loc3, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Mines take away your points");

	bounds.y += 40;
	fontBounds.y += 40;
	CIwSVec2 loc4(bounds.x + (iconWidth - g_pTextureBolt->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureBolt->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureBolt, loc4, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Shake your phone to stun fireflies");

	IwGxLightingOff();
	*/
}