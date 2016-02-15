#ifndef IW_MAPGAME_H
#define IW_MAPGAME_H

#include "IwSingleton.h"
#include "Main.h"
#include "s3e.h"
#include "s3eSocket.h"
#include "s3eLocation.h"
#include "IwHTTP.h"
#include "IwTexture.h"
#include "IwMaterial.h"
#include "IwJpeg.h"
#include "IwImage.h"
#include "Iw2D.h"
#include "IwGxPrint.h"
#include "IGameState.h"
#include "LiveMaps.h"
#include "CoordinateScaler.h"
#include "IGameHandler.h"
#include "IwMultiplayerHandler.h"

struct GameStatePair
{
	GPS_GameState gameState;
	IGameState* pGameState;
};

class CIwMapGame : public IGameHandler
{
public:
	CIwMapGame(void);
	~CIwMapGame(void);

	void Exit();
	void Init();
	void ShutDown();
	bool Update();
	void Render();
	void SetStaticContent(char*);
	void SetGameState(GPS_GameState gameState, EAnimDirection direction);
	IGameState* GetGameState(GPS_GameState gameState);
	void AddHighScore(int newScore, uint64 newTime, double distanceTravelled);
	void SetActiveUI(CIwUIElement* pDialogTemplate, IIwUIEventHandler* pEventHandler);
	bool UpdatePositionFromMouseAndKeyboard(CIwSVec2& clickPos, bool* pbClicked);
	int GetFramesPerSecond();

	void SetGameEngine(IGameEngine* pGameEngine);
	IGameEngine* GetGameEngine();

	bool GetLocation(s3eLocation& location);
	bool IsGpsActive();

	void ClearTilt();
	void StartTrackingDistance() { g_distanceTravelled = 0; g_bTrackingDistance = true; }
	void StopTrackingDistance() { g_bTrackingDistance = false; }
	virtual double GetDistanceTravelled() { return g_distanceTravelled; }

private:
	static int32 GotHeaders(void* pDownloaderVoid, void* pMapTileVoid);
	static int32 GotData(void* pDownloaderVoid, void* pMapTileVoid);

	IGameEngine* g_pGameEngine;
	CIwHTTP* g_downloader;
	s3eLocation	gPrevLocation;
	s3eLocation	gLocation;
	s3eResult	gError;

	int32		g_Width;
	int32		g_Height;
	int			g_cursorIter;

	bool g_bInProgress;
	bool g_bNeedsExit;
	
	CIwUIElement* g_pDialogCurrent;
	std::list<GameStatePair*> g_gameStates;
	GameStatePair* g_gameState;
	EAnimDirection g_direction;
	
	// Tilt members
	bool g_bUseTilt;
	int g_xTiltOffset;
	int g_yTiltOffset;
	float g_xTiltVelo;
	float g_yTiltVelo;
	float g_xTiltPos;
	float g_yTiltPos;

	double g_distanceTravelled;
	bool g_bTrackingDistance;

	uint64 g_lastGpsLookup;
	uint64 g_lastAccelLookup;
	s3eLocation g_lastLocation;
};

IW_SINGLETON_EXTERN(MapGame);

#endif