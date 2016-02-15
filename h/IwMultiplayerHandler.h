/*
 * This file is part of the Airplay SDK Code Samples.
 *
 * Copyright (C) 2001-2010 Ideaworks3D Ltd.
 * All Rights Reserved.
 *
 * This source code is intended only as a supplement to Ideaworks Labs
 * Development Tools and/or on-line documentation.
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 */
//------------------------------------------------------------------------------
/*!
	\file	IwMapImage.h
*/
//------------------------------------------------------------------------------

#ifndef IW_MULTIHANDLER_H
#define IW_MULTIHANDLER_H

#include "IwSingleton.h"
#include "s3e.h"
#include "s3eSocket.h"
#include "IwArray.h"
#include "IwGx.h"
#include "IWHTTPQUEUE.h"
#include "IGameHandler.h"

enum MultiplayerMode
{
	MPM_INIT,
	MPM_M_CREATING,
	MPM_M_WAITING_FOR_READY,
	MPM_IN_GAME,
	MPM_S_JOINING,
	MPM_S_WAITING_FOR_CONFIRM,
	MPM_S_WAITING_FOR_READY,
	MPM_S_WAITING_FOR_START,
	MPM_GAME_OVER
};

enum MultiplayerGameMessage
{
    MPGM_Create = 0,
    MPGM_CreateResponse = 1,
    MPGM_List = 10,
    MPGM_ListResponse = 11,
    MPGM_Join = 20,
    MPGM_JoinResponse = 21,
    MPGM_GetGameDetails = 30,
    MPGM_GetGameDetailsResponse = 31,
    MPGM_AcceptUser = 40,
    MPGM_AcceptUserResponse = 41,
    MPGM_RejectUser = 50,
    MPGM_RejectUserResponse = 51,
    MPGM_Start = 52,
    MPGM_StartResponse = 53,
    MPGM_SetReady = 60,
    MPGM_SetReadyResponse = 61,
    MPGM_PushState = 70,
    MPGM_PushStateResponse = 71,
    MPGM_EndGame = 90,
    MPGM_EndGameResponse = 91,
    MPGM_NOP = 99,
    MPGM_Disconnect = 100
};

#define GAME_PORT 81

typedef void (*MessageReceivedCallback)(const char * Result, uint32 ResultLen, void* userData);
typedef void (*ConnectionChangedCallback)(bool success, const char* szStatus, void* userData);
typedef void (*ModeChangedCallback)(MultiplayerMode mode, void* userData);

class CIwMultiplayerHandler
{
public:
	struct MessageNotification
	{
		uint32 identifier;
		MessageReceivedCallback pCallback;
		void* userData;
	};
	struct User
	{
		uint uiIndex;
		char szName[150];
		char szDevice[150];
		bool Accepted;
		bool Ready;
		double Latitude;
		double Longitude;
		bool IsMe;
		int Score;
		s3eInetAddress Address;
		uint32 LastPacket;
		int32 UserData;
	};

	CIwMultiplayerHandler();
	~CIwMultiplayerHandler();

	void Send(uint identifier, char* szData, int len, bool sendOnce = false);
	void RegisterCallback(uint32 identifier, MessageReceivedCallback pCallback, void* userData);
	void Update();
	void Flush();
	void StartMultiplayerGame(ConnectionChangedCallback pCallback, void* userData);
	void JoinMultiplayerGame(ConnectionChangedCallback pCallback, void* userData);
	void SetUserScore(CIwMultiplayerHandler::User* pUser, int score);
	void StartSinglePlayerGame()
	{
		EndGame();
		g_users.append(g_pMe);
		g_pMe->uiIndex = 0;
		g_pMe->Score = 0;
	}

	void SetReady();
	void StartGame();
	void EndGame();
	CIwArray<User*> ListUsers();
	User* GetMe();
	void AcceptUser(User* pUser);
	void RejectUser(User* pUser);
	void GetDeviceId(char szBuffer[150]);
	static void EscapeUrl(char* szUrl);

	int32 RequestIdentifier();
	bool IsMaster();
	bool IsEveryoneReady() { return (!g_pSock || g_bAllUsersReady); }
	bool IsReady() { return g_bIsReady; }
	bool IsGameReady() { return g_bGameReady; }
	bool ReceivedMapData() { return g_bReceivedMapData; }
	bool InSocketMode() { return g_bSocketMode; }
	bool IsMultiplayer();

	void SetGameHandler(IGameHandler* pHandler)
	{
		g_pHandler = pHandler;
	}

	CIwVec2* GetMasterDimensions() { return &g_masterDimensions; }
	CIwFVec2* GetScaleRatio() { return &g_scaleRatio; }

	void RegisterDisconnectCallback(ConnectionChangedCallback callback, void* userData);
	void RegisterModeChangedCallback(ModeChangedCallback callback, void* userData);
	void Post(MultiplayerGameMessage message, char* szQueryString, char* szBuffer, int bufferOffset, void* pData, pCallback callback, pCallbackError callbackError, int timeout);
	void Get(MultiplayerGameMessage message, char* szQueryString, void* pData, pCallback callback, pCallbackError callbackError, int timeout);

private:

	void HandleDisconnect(char* szReason);
	void SetGameId(const char* szGameId);
	void SetMode(MultiplayerMode mode);

	static void GotGameDetails(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void GotGameState(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void GotJoinState(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void GotError(void* pThis);

	static void GotSetReady(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void SetReadyWebServiceError(void* pThis);

	static void GotConfirmUser(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void ConfirmUserWebServiceError(void* pThis);

	static void GotEndGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void EndGameWebServiceError(void* pThis);

	static void GotStartGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void StartGameWebServiceError(void* pThis);

	static void GotCreateGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void CreateGameWebServiceError(void* pThis);
	
	static void GotListGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void ListGameWebServiceError(void* pThis);
	
	static void GotJoinGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen);
	static void JoinGameWebServiceError(void* pThis);

	static int32 SocketConnected(s3eSocket* s, void* systemData, void* userData);

	struct ModeChangedItem
	{
		ModeChangedCallback callback;
		void* userData;
	};

	ConnectionChangedCallback g_pDisconnectCallback;
	ConnectionChangedCallback g_pStartCallback;
	ConnectionChangedCallback g_pJoinCallback;
	CIwArray<ModeChangedItem> g_modeCallbacks;
	CIwArray<MessageNotification> g_Messages;
	bool g_bMaster, g_bInRequest, g_bGameReady, g_bAccepted, g_bReceivedMapData, g_bAllUsersReady, g_bIsReady, g_bSocketMode;

	void* g_disconnectUserData;
	void* g_startUserData;
	void* g_joinUserData;
	char g_szGameId[50];

	MultiplayerMode g_mode;
	CIwArray<User*> g_users;

	uint32 g_identifier;
	uint64 g_lastUpdate;
	uint64 g_lastSuccess;
	uint64 g_lastSend;

	CIwVec2 g_masterDimensions;
	CIwFVec2 g_scaleRatio;

	CIwTexture* g_pConnectionTexture;

	char g_szDeviceId[50];

	s3eSocket* g_pSock;

	bool g_bNeedFlush;
	s3eInetAddress g_addr;
	uint32 g_lastPacketTimeStamp;
	uint16 g_port;
	uint32 g_messageIdentifier;

	User* g_pMe;
	IGameHandler* g_pHandler;
};

IW_SINGLETON_EXTERN(MultiplayerHandler);

#endif
