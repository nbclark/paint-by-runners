#include "Main.h"
#include "MapGame.h"
#include "s3e.h"
#include "IwUI.h"
#include "MessageBox.h"
#include "IntroGameState.h"
#include "CreateMapGameState.h"
#include "SelectMapGameState.h"
#include "SelectGameGameState.h"
#include "ActiveGameState.h"
#include "HighScoreGameState.h"
#include "StaticContentGameState.h"


s3eFile* g_pFile = NULL;

CIwMapGame::CIwMapGame()
{
	g_bNeedsExit = false;

	g_direction = AnimDir_Initial;
	g_pFile = s3eFileOpen("coords.txt", "w");
	g_bUseTilt = false;
	g_bTrackingDistance = false;
	g_lastGpsLookup = 0;
	g_xTiltOffset = g_yTiltOffset = 0;
}

CIwMapGame::~CIwMapGame()
{
	std::list<GameStatePair*>::iterator iter = g_gameStates.begin();

	while (iter != g_gameStates.end())
	{
		GameStatePair* pPair = *iter;
		delete pPair->pGameState;
		delete pPair;
		iter++;
	}
	s3eFileFlush(g_pFile);
	s3eFileClose(g_pFile);
}

void CIwMapGame::Init()
{
	g_gameState = NULL;

	GameStatePair* pIntro = new GameStatePair;
	pIntro->gameState = GPS_GameState_Intro;
	pIntro->pGameState = new IntroGameState(this);
	pIntro->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pIntro);

	GameStatePair* pActive = new GameStatePair;
	pActive->gameState = GPS_GameState_Active;
	pActive->pGameState = new ActiveGameState;
	pActive->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pActive);

	GameStatePair* pSelectGame = new GameStatePair;
	pSelectGame->gameState = GPS_GameState_SelectGame;
	pSelectGame->pGameState = new SelectGameGameState(this);
	pSelectGame->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pSelectGame);

	GameStatePair* pHighScore = new GameStatePair;
	pHighScore->gameState = GPS_GameState_HighScore;
	pHighScore->pGameState = new HighScoreGameState();
	pHighScore->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pHighScore);

	GameStatePair* pStaticContent = new GameStatePair;
	pStaticContent->gameState = GPS_GameState_StaticContent;
	pStaticContent->pGameState = new StaticContentGameState();
	pStaticContent->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pStaticContent);

	GameStatePair* pCreateMap = new GameStatePair;
	pCreateMap->gameState = GPS_GameState_CreateMap;
	pCreateMap->pGameState = new CreateMapGameState();
	pCreateMap->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pCreateMap);

	GameStatePair* pSelectMap = new GameStatePair;
	pSelectMap->gameState = GPS_GameState_SelectMap;
	pSelectMap->pGameState = new SelectMapGameState();
	pSelectMap->pGameState->SetGameHandler(this);
	g_gameStates.push_back(pSelectMap);

	g_pDialogCurrent = NULL;

	g_cursorIter = 1;

	// Get the current location
	if (S3E_RESULT_SUCCESS != s3eLocationGet(&gLocation))
	{
		//47.76216699999999
		//-122.160358
		gLocation.m_Latitude = 47.7710083;
		gLocation.m_Longitude = -122.1588533;
	}

	SetGameState(GPS_GameState_Intro, AnimDir_Initial);
}

void CIwMapGame::Exit()
{
	g_bNeedsExit = true;
}

void CIwMapGame::ClearTilt()
{
	g_xTiltOffset = g_yTiltOffset = 0;
	g_xTiltVelo = g_yTiltVelo = 0;

	g_xTiltPos = Iw2DGetSurfaceWidth() / 2.0f;
	g_yTiltPos = Iw2DGetSurfaceHeight() / 2.0f;
}

bool CIwMapGame::IsGpsActive()
{
	if (s3eLocationAvailable())
	{
		s3eLocation loc;
		
		if (S3E_RESULT_SUCCESS == s3eLocationGet(&loc))
		{
			return true;
		}
	}
	s3eLocationStart();

	return false;
}

void CIwMapGame::SetGameEngine(IGameEngine* pGameEngine)
{
	g_pGameEngine = pGameEngine;
}

IGameEngine* CIwMapGame::GetGameEngine()
{
	return g_pGameEngine;
}

bool CIwMapGame::GetLocation(s3eLocation& location)
{
	uint64 timer = s3eTimerGetMs();

	if (g_lastGpsLookup == 0 || (timer - g_lastGpsLookup) > 500)
	{
		if (S3E_RESULT_SUCCESS != s3eLocationGet(&location))
		{
			return false;
		}
		s3eFilePrintf(g_pFile, "%lld\t%lld\t%4.9f\t%4.9f\t%4.9f\t%4.9f\t%4.9f\r\n", timer, location.m_TimeStampUTC, location.m_Longitude, location.m_Latitude, location.m_Altitude, location.m_HorizontalAccuracy, location.m_VerticalAccuracy);
		s3eFileFlush(g_pFile);

		if (g_bTrackingDistance)
		{
			g_distanceTravelled += LiveMaps::CalculateDistance(g_lastLocation, location);
		}

		g_lastLocation = location;
		g_lastGpsLookup = timer;
		return true;
	}
	else
	{
		if (g_lastGpsLookup)
		{
			location = g_lastLocation;
			return true;
		}
		return false;
	}
}

int CIwMapGame::GetFramesPerSecond()
{
	return FRAMES_PER_S;
}

void CIwMapGame::AddHighScore(int newScore, uint64 newTime, double distanceTravelled)
{
	HighScoreGameState* pHighScore = (HighScoreGameState*)GetGameState(GPS_GameState_HighScore);
	pHighScore->AddScore(newScore, newTime, distanceTravelled);
}

void CIwMapGame::SetStaticContent(char* szPanel)
{
	StaticContentGameState* pStaticContent = (StaticContentGameState*)GetGameState(GPS_GameState_StaticContent);

	pStaticContent->SetContent(szPanel);
}


IGameState* CIwMapGame::GetGameState(GPS_GameState gameState)
{
	std::list<GameStatePair*>::iterator iter = g_gameStates.begin();
	while (iter != g_gameStates.end())
	{
		GameStatePair* pPair = *iter;
		if (pPair->gameState == gameState)
		{
			return pPair->pGameState;
		}
		iter++;
	}
	return NULL;
}

void CIwMapGame::SetGameState(GPS_GameState gameState, EAnimDirection direction)
{
	g_direction = direction;

	if (g_gameState)
	{
		g_gameState->pGameState->DeActivate();
		g_gameState = NULL;
	}

	if (gameState == GPS_GameState_Active)
	{
		ClearTilt();
	}


	std::list<GameStatePair*>::iterator iter = g_gameStates.begin();
	while (iter != g_gameStates.end())
	{
		GameStatePair* pPair = *iter;
		if (pPair->gameState == gameState)
		{
			g_gameState = pPair;
			pPair->pGameState->Activate();
		}
		iter++;
	}
}

void CIwMapGame::SetActiveUI(CIwUIElement* pDialogTemplate, IIwUIEventHandler* pEventHandler)
{
	if (g_pDialogCurrent)
	{
		g_pDialogCurrent->SetVisible(false);
	}

	if (pEventHandler)
	{
		IwGetUIController()->AddEventHandler(pEventHandler);
	}
	g_pDialogCurrent = pDialogTemplate;
	IwGetUIAnimManager()->StopAnim(pDialogTemplate);
	pDialogTemplate->SetVisible(false);

	switch (g_direction)
	{
		case AnimDir_Left :
		{
			IwGetUIAnimManager()->PlayAnim("slideInAnim", pDialogTemplate, false);
		}
		break;
		case AnimDir_Right :
		{
			IwGetUIAnimManager()->PlayAnim("slideOutAnim", pDialogTemplate, false);
		}
		break;
		default :
		{
			IwGetUIAnimManager()->PlayAnim("zoominAnim", pDialogTemplate, false);
			pDialogTemplate->SetVisible(true);
		}
		break;
	}
}

//-----------------------------------------------------------------------------
void CIwMapGame::ShutDown()
{
	//s3eLocationStop();
}

bool CIwMapGame::Update()
{
	if (g_gameState)
	{
		g_gameState->pGameState->Update();
	}
	return !g_bNeedsExit;
}

void CIwMapGame::Render()
{
	if (g_gameState)
	{
		g_gameState->pGameState->Render();

		if (g_pDialogCurrent)
		{
			if (!g_pDialogCurrent->IsVisible())
			{
				g_pDialogCurrent->SetVisible(true);
			}
		}
	}
}