#pragma once
#include "IwUI.h"
#include "IGameEngine.h"
#include "Utils.h"

typedef enum GPS_GameState
{
	GPS_GameState_Intro = 0,
	GPS_GameState_SelectMap = 1,
	GPS_GameState_CreateMap = 2,
	GPS_GameState_Active = 3,
	GPS_GameState_Pause = 4,
	GPS_GameState_SelectGame = 5,
	GPS_GameState_HighScore = 6,
	GPS_GameState_StaticContent = 7,
	GPS_GameState_End = 8
} GPS_GameState;

enum EAnimDirection
{
	AnimDir_Initial = 0,
	AnimDir_Left = 1,
	AnimDir_Right = 2
};

class IGameHandler
{
public:
	virtual void SetStaticContent(char*) {}
	virtual void SetGameState(GPS_GameState gameState, EAnimDirection direction) {}
	virtual void SetActiveUI(CIwUIElement* pDialogTemplate, IIwUIEventHandler* pEventHandler) {}
	virtual void AddHighScore(int newScore, uint64 newTime, double distanceTravelled) {}

	virtual void StartTrackingDistance() {}
	virtual void StopTrackingDistance() {}
	virtual double GetDistanceTravelled() { return 0; }

	virtual int GetFramesPerSecond() { return 30; }

	virtual void Exit() {}

	virtual void SetGameEngine(IGameEngine* pGameEngine) {}
	virtual IGameEngine* GetGameEngine() { return NULL; }

	virtual bool IsGpsActive() { return false; }

	Region* GetBoundingRegion()
	{
		return &g_region; 
	}
	void SetBoundingRegion(char* szRegion, Region* pRegion)
	{
		strcpy(g_szRegion, szRegion);
		
		g_region.Clear();

		if (pRegion)
		{
			g_region.SetBoundingBox(pRegion->g_tlBB, pRegion->g_brBB);

			for (int i = 0; i < pRegion->g_triangles.size(); ++i)
			{
				g_region.Add(pRegion->g_triangles[i].A, pRegion->g_triangles[i].B);
			}
		}
	}
	char* GetLoadedRegion() { return g_szRegion; }

protected:
	Region g_region;
	char g_szRegion[1000];
};