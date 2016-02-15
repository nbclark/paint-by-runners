#pragma once
#include "gamestate.h"
#include "IGameObjects.h"

// These events represent game state changes
typedef enum PicnicGamesEvent
{
	PGE_GAME			= 0,
	PGE_SCORESINGLE		= 1,
	PGE_SCOREMULTI		= 2,
} PicnicGamesEvents;

struct PicnicGamesMessage
{
	char ev;
	int16 dataA;
	int16 dataB;
	int16 score1;
	int16 score2;
	int16 score3;
	int16 score4;
	int16 score5;
	int16 score6;
	int16 score7;
	int16 score8;
	int16 score9;
};

inline void PrepareSend_PicnicGamesMessage(PicnicGamesMessage& host, PicnicGamesMessage& network)
{
	network.dataA = s3eInetHtons(host.dataA);
	network.dataB = s3eInetHtons(host.dataB);
	network.score1 = s3eInetHtons(host.score1);
	network.score2 = s3eInetHtons(host.score2);
	network.score3 = s3eInetHtons(host.score3);
	network.score4 = s3eInetHtons(host.score4);
	network.score5 = s3eInetHtons(host.score5);
	network.score6 = s3eInetHtons(host.score6);
	network.score7 = s3eInetHtons(host.score7);
	network.score8 = s3eInetHtons(host.score8);
	network.score9 = s3eInetHtons(host.score9);
}

inline void PrepareReceive_PicnicGamesMessage(PicnicGamesMessage* pNetwork, PicnicGamesMessage* pHost)
{
	pHost->dataA = s3eInetNtohs(pNetwork->dataA);
	pHost->dataB = s3eInetNtohs(pNetwork->dataB);
	pHost->score1 = s3eInetNtohs(pNetwork->score1);
	pHost->score2 = s3eInetNtohs(pNetwork->score2);
	pHost->score3 = s3eInetNtohs(pNetwork->score3);
	pHost->score4 = s3eInetNtohs(pNetwork->score4);
	pHost->score5 = s3eInetNtohs(pNetwork->score5);
	pHost->score6 = s3eInetNtohs(pNetwork->score6);
	pHost->score7 = s3eInetNtohs(pNetwork->score7);
	pHost->score8 = s3eInetNtohs(pNetwork->score8);
	pHost->score9 = s3eInetNtohs(pNetwork->score9);
}

typedef enum PicnicGamesState
{
	PGS_INIT			= 0,
	PGS_WAITING			= 1,
	PGS_INGAME			= 2,
	PGS_END				= 3
} PicnicGamesStates;

class ActiveGameState :
	public GameState
{
public:
	ActiveGameState(void);
	~ActiveGameState(void);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();
	
	virtual bool IsScore() { return g_bIsScore; }

private:
	void RenderBackground();
	void OnClickResume(CIwUIElement* Clicked);
	void StartNextGame();
	void ShowScore();
	void HideScore();
	void SetUserScore(int16 userId, int16 score, bool setMe);

	static void ReceiveStatusUpdate(const char * Result, uint32 ResultLen, void* userData);

	static void MultiplayerDisconnect(bool success, const char* szStatus, void* userData);
	static void MultiplayerModeChanged(MultiplayerMode mode, void* userData);

	int g_cursorIter;
	std::list<s3eLocation> g_downPos;

	bool g_bMouseDown, g_bRenderGame, g_bIsScore, g_bGameOver;

	int g_iAnimationCount;
	int g_iStartGameCount;

	IGameEngine* g_pGameEngine;

	CIwUIElement* g_pTitleBar;
	CIwGxFont* g_pFont, *g_pFontLarge;
	float g_dAlpha;
	uint64 g_uiStartTime, g_mpLastFlush;

	CScoreKeeper* g_pScoreKeeper;
	CContentBlock* g_pContentBlock;
	CCountdownTimer* g_pTimer;
	CBarGraph* g_barGraph;

	CIwTexture* g_pBackground;
	CIwTexture* g_pReady;
	CIwTexture* g_pSet;
	CIwTexture* g_pGo;

	CIwArray<IGameEngine*> g_games;
	CIwArray<int> g_gameIndices;
	int g_iGameIndex;
	int16 g_iTotalScore;
	PicnicGamesState g_gameState;

	int g_iIntroCount;
	int g_iIntroReadyCount;
	int g_iIntroSetCount;

	bool g_allUsersReady;
};
