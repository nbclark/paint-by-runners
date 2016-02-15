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
#include "IwMultiplayerHandler.h"
#include "IwNotificationHandler.h"
#include "IwUtil.h"
#include "Iw2D.h"
#include "IwDebug.h"
#include "IwRuntime.h"
#include "IwHttpQueue.h"
#include "s3eLocation.h"
#include "tinyxml.h"
#include "Utils.h"
#include "IwDebug.h"
#include "MessageBox.h"
#include "main.h"


// http://mobilesrc.appspot.com

IW_SINGLETON_INSTANTIATE(MultiplayerHandler);

#define SOCKETBUFFERSIZE 15000
char g_szOfflineBuffer[SOCKETBUFFERSIZE];
char g_szBuffer[SOCKETBUFFERSIZE];
char g_szRecvBuffer[SOCKETBUFFERSIZE];
char g_szSendBuffer[SOCKETBUFFERSIZE];
int g_offlineBufferOffset = 1;
int g_bufferOffset = 1;

int g_iSend = 0;
int g_iRecv = 0;

CIwMultiplayerHandler::CIwMultiplayerHandler()
{
	IW_SINGLETON_SET(MultiplayerHandler);
	g_identifier = 0;

	strcpy(g_szGameId, "");
	strcpy(g_szDeviceId, "");
	g_mode = MPM_INIT;
	g_bInRequest = false;
	g_bAccepted = false;
	g_bGameReady = false;
	g_bReceivedMapData = false;
	g_lastUpdate = 0;
	g_bIsReady = false;
	g_lastSuccess = 0;
	g_lastSend = 0;

	g_pDisconnectCallback = NULL;
	g_pConnectionTexture = (CIwTexture*)IwGetResManager()->GetResNamed("connection", "CIwTexture");
	g_bSocketMode = true;
	g_pSock = NULL;

	g_bNeedFlush = false;

	g_pMe = new User;
	const char* szName = s3eDeviceGetString(S3E_DEVICE_NAME);
	GetDeviceId(g_pMe->szDevice);
	strncpy(g_pMe->szName, szName, sizeof(g_pMe->szDevice));
	g_pMe->IsMe = true;
	g_pMe->uiIndex = 0;
	g_pMe->Score = 0;

	//g_mode = MPM_M_WAITING_FOR_READY;
	//SetGameId("agltb2JpbGVzcmNyDAsSBEdhbWUYgNMBDA");
}

CIwMultiplayerHandler::~CIwMultiplayerHandler()
{	
	g_Messages.clear();
	for (uint32 i = 0; i < g_users.size(); i++)
	{
		if (g_users[i] != g_pMe)
		{
			delete g_users[i];
		}
	}
	delete g_pMe;
	g_users.clear();
}

void CIwMultiplayerHandler::GetDeviceId(char g_szBuffer[150])
{
	if (strlen(g_szDeviceId) == 0)
	{
		const char* szId = s3eDeviceGetString(S3E_DEVICE_UNIQUE_ID);

		if (strlen(szId) == 0)
		{
			szId = s3eDeviceGetString(S3E_DEVICE_PHONE_NUMBER);
		}
		if (strlen(szId) == 0)
		{
			szId = s3eDeviceGetString(S3E_DEVICE_NAME);
		}
		strncpy(g_szDeviceId, szId, 150);
		
		IwRandSeed((int32)s3eTimerGetMs());

		char szRand[50];
		sprintf(szRand, "_%x", IwRandMinMax(0, 100));
		strcat(g_szDeviceId, szRand);

		EscapeUrl(g_szDeviceId);
	}
	strncpy(g_szBuffer, g_szDeviceId, 150);

	if (IsMaster())
	{
		//strcat(g_szBuffer, "_master");
	}
	else
	{
		//strcat(g_szBuffer, "_slave");
	}
}

void CIwMultiplayerHandler::EscapeUrl(char* szUrl)
{
	char* szReplace = &szUrl[0];
	while (szReplace[0])
	{
		if (szReplace[0] == ' ')
		{
			szReplace[0] = '_';
		}
		szReplace++;
	}
}

void CIwMultiplayerHandler::SetUserScore(CIwMultiplayerHandler::User* pUser, int score)
{
	for (uint32 i = 0; i < g_users.size(); ++i)
	{
		if (0 == stricmp(pUser->szDevice, g_users[i]->szDevice))
		{
			g_users[i]->Score = score;
		}
	}
}

void CIwMultiplayerHandler::RegisterDisconnectCallback(ConnectionChangedCallback callback, void* userData)
{
	g_pDisconnectCallback = callback;
	g_disconnectUserData = userData;
}

void CIwMultiplayerHandler::RegisterModeChangedCallback(ModeChangedCallback callback, void* userData)
{
	ModeChangedItem item;
	item.callback = callback;
	item.userData = userData;

	g_modeCallbacks.append(item);
}

void CIwMultiplayerHandler::SetMode(MultiplayerMode mode)
{
	if (g_mode != mode)
	{
		g_mode = mode;

		for (uint32 i = 0; i < g_modeCallbacks.size(); i++)
		{
			g_modeCallbacks[i].callback(g_mode, g_modeCallbacks[i].userData);
		}
	}
}

bool CIwMultiplayerHandler::IsMultiplayer()
{
	return (0 < strlen(g_szGameId)); 
}

bool CIwMultiplayerHandler::IsMaster()
{
	return (g_bMaster || !IsMultiplayer());
}

int32 CIwMultiplayerHandler::RequestIdentifier()
{
	g_identifier++;

	if (g_identifier == 53)
	{
		s3eDeviceYield(0);
	}
	return g_identifier;
}

void CIwMultiplayerHandler::RegisterCallback(uint32 identifier, MessageReceivedCallback pCallback, void* userData)
{
	MessageNotification notif;
	notif.identifier = identifier;
	notif.pCallback = pCallback;
	notif.userData = userData;

	g_Messages.append(notif);
}


void CIwMultiplayerHandler::Send(uint identifier, char* szData, int len, bool sendOnce)
{
	if (g_mode == MPM_IN_GAME)
	{
		if (!sendOnce && g_bInRequest)
		{
			return;
		}

		char* szWriteBuffer = g_szBuffer;
		int bufferCount = g_bufferOffset;

		// make sure that we have enough space to write
		IwAssert("FF", (bufferCount+len+2) < SOCKETBUFFERSIZE);
		IwAssert("FF", (len < 0xFF));
		IwAssert("FF", (identifier < 0xFF));
		IwAssert("FF", ((szWriteBuffer[0]+1) < 0xFF));

		// MESSAGECOUNT|IDENTIFIER|MESSAGELENGTH|DATA...
		// We are adding a message. increment the count
		szWriteBuffer[0]++;

		// we write into the buffer at our offset
		char* szOffsetBuffer = szWriteBuffer+bufferCount;
		szOffsetBuffer[0] = (char)identifier;
		szOffsetBuffer++;
		szOffsetBuffer[0] = (char)len;
		szOffsetBuffer++;
		memcpy(szOffsetBuffer, szData, len);

		// Add 2 bytes, plus our message length
		bufferCount += (2 + len);
		g_bufferOffset = bufferCount;

		if (sendOnce)
		{
			g_bNeedFlush = true;
		}
	}
	else
	{
		s3eDeviceYield(0);
	}
}

void CIwMultiplayerHandler::Flush()
{
	g_bNeedFlush = false;

	if (g_mode == MPM_IN_GAME)
	{
		uint64 timer = s3eTimerGetMs();

		if (!g_bInRequest || g_bSocketMode)
		{
			if (g_bufferOffset > 1)
			{
				char szRequest[500];
				char szId[150];
				GetDeviceId(szId);
				strcpy(szRequest, "");

				g_bInRequest = !g_bSocketMode;

				const char * szType = IsMaster() ? "master" : "slave";

				// push our state and read the result
				sprintf(szRequest, "%spush?&game=%s&id=%s", szType, g_szGameId, szId);

				EscapeUrl(szRequest);
				Post(MPGM_PushState, szRequest, g_szBuffer, g_bufferOffset, this, GotGameState, GotError, 4000);
				g_lastUpdate = timer;
			}
		}
	}
}

/*
void CIwMultiplayerHandler::Send(uint identifier, char* szData, int len, bool sendOnce)
{
	if (g_mode == MPM_IN_GAME)
	{
		if (!sendOnce && g_bInRequest)
		{
			return;
		}

		char* szWriteBuffer = g_szBuffer;
		int bufferCount = g_bufferOffset;
		if (sendOnce)
		{
			szWriteBuffer = g_szOfflineBuffer;
			bufferCount = g_offlineBufferOffset;
		}

		// make sure that we have enough space to write
		IwAssert("FF", (bufferCount+len+10) < SOCKETBUFFERSIZE);
		IwAssert("FF", (len < 0xFF));

		// MESSAGECOUNT|IDENTIFIER|MESSAGELENGTH|DATA...
		// We are adding a message. increment the count
		szWriteBuffer[0]++;

		// we write into the buffer at our offset
		char* szOffsetBuffer = szWriteBuffer+bufferCount;
		szOffsetBuffer[0] = (char)identifier;
		szOffsetBuffer++;
		szOffsetBuffer[0] = (char)len;
		szOffsetBuffer++;
		memcpy(szOffsetBuffer, szData, len);

		// Add 2 bytes, plus our message length
		bufferCount += (2 + len);

		if (sendOnce)
		{
			g_offlineBufferOffset = bufferCount;
		}
		else
		{
			g_bufferOffset = bufferCount;
		}
	}
	else
	{
		s3eDeviceYield(0);
	}
}

void CIwMultiplayerHandler::Flush()
{
	if (g_mode == MPM_IN_GAME)
	{
		uint64 timer = s3eTimerGetMs();

		if (!g_bInRequest || g_bSocketMode)
		{
			if (g_bufferOffset > 1)
			{
				char szRequest[500];
				char szId[50];
				GetDeviceId(szId);
				strcpy(szRequest, "");

				g_bInRequest = !g_bSocketMode;

				const char * szType = IsMaster() ? "master" : "slave";

				// push our state and read the result
				sprintf(szRequest, "%spush?&game=%s&id=%s", szType, g_szGameId, szId);
				
				if (g_szOfflineBuffer[0] > 0)
				{
					g_szBuffer[0] += g_szOfflineBuffer[0];

					char* szEnd = g_szBuffer + g_bufferOffset;

					memcpy(szEnd, g_szOfflineBuffer+1, g_offlineBufferOffset-1);
					g_bufferOffset += g_offlineBufferOffset - 1;

					g_offlineBufferOffset = 1;
					g_szOfflineBuffer[0] = 0;
				}

				EscapeUrl(szRequest);
				Post(MPGM_PushState, szRequest, g_szBuffer, g_bufferOffset, this, GotGameState, GotError, 4000);
				g_lastUpdate = timer;
			}
		}
	}
}
*/

void CIwMultiplayerHandler::Post(MultiplayerGameMessage message, char* szQueryString, char* szDataBuffer, int bufferOffset, void* pData, pCallback callback, pCallbackError callbackError, int timeout = 5000)
{
	if (g_bSocketMode)
	{
		int offset = 0;

		uint32 networkQueryStringLength = s3eInetHtonl(g_messageIdentifier++);
		memcpy((void*)&g_szSendBuffer[offset], &networkQueryStringLength, sizeof(uint32));
		offset += sizeof(uint32);

		if (message == MPGM_Create || message == MPGM_List || message == MPGM_Join)
		{
			if (NULL == g_pSock)
			{
				int64 utcTime = s3eTimerGetUTC();
				
				if (s3eDebugIsDebuggerPresent())
				{
					g_port = 8001;
				}
				else
				{
					g_port = 8000 + (uint16)((utcTime >> 3) & 127);
				}
			}

			s3eInetAddress localAddr;
			memset(&localAddr, 0, sizeof(localAddr));
			s3eInetLookup(s3eSocketGetString(S3E_SOCKET_HOSTNAME), &localAddr, 0, 0);
			localAddr.m_Port = s3eInetHtons(g_port);

			char* szColon = strrchr(localAddr.m_String, ':');
			if (szColon)
			{
				szColon[0] = 0;
			}

			char szAddress[100];
			sprintf(szAddress, "&address=%s&port=%d", localAddr.m_String, g_port);
			strcat(szQueryString, szAddress);

			if (NULL == g_pSock)
			{
				g_lastSuccess = s3eTimerGetMs();

				g_pSock = s3eSocketCreate(S3E_SOCKET_UDP, 0);
				
				memset(&g_addr, 0, sizeof(g_addr));
				//g_addr.m_Port = s3eInetHtons(g_port);
				
				s3eResult bindResult = s3eSocketBind(g_pSock, &g_addr, 1);

				//if (bindResult != S3E_RESULT_SUCCESS)
				//{
				//	MessageBox::Show("Error binding socket", "", "OK", "Cancel", NULL, this);
				//}

				s3eInetLookup(RUNAWAYGAME_SERVER, &g_addr, NULL, NULL);
				//s3eInetLookup("localhost", &g_addr, NULL, NULL);
				g_addr.m_Port = s3eInetHtons(RUNAWAYGAME_PORT);
				g_lastPacketTimeStamp = 0;
			}
		}

		g_szSendBuffer[offset] = (char)message;
		offset++;

		// Get the message length ...
		int16 queryStringLength = strlen(szQueryString);
		int16 messageLength = 2 + queryStringLength + bufferOffset;

		if (bufferOffset)
		{
			messageLength += 2;
		}

		if (message == MPGM_PushState)
		{
			messageLength = bufferOffset;
		}

		if (message != MPGM_PushState)
		{
			int16 networkQueryStringLength = s3eInetHtons(queryStringLength);
			memcpy((void*)&g_szSendBuffer[offset], &networkQueryStringLength, sizeof(int16));
			offset += sizeof(int16);
			
			memcpy((void*)&g_szSendBuffer[offset], szQueryString, queryStringLength);
			offset += queryStringLength;
		
			if (bufferOffset)
			{
				int16 networkBufferOffset = s3eInetHtons(bufferOffset);
				memcpy((void*)&g_szSendBuffer[offset], &networkBufferOffset, sizeof(int16));
				offset += sizeof(int16);
			}
		}
		
		if (bufferOffset)
		{
			memcpy((void*)&g_szSendBuffer[offset], szDataBuffer, bufferOffset);
			offset += bufferOffset;
		}

		if (message == MPGM_PushState)
		{
			// reset our push state
			g_szBuffer[0] = 0;
			g_bufferOffset = 1;
		}

		int32 bytesSent = 0;
		if (g_mode == MPM_IN_GAME && message == MPGM_PushState && false)
		{
			// If we are in game, send gamestate directly to devices
			for (int i = 0; i < g_users.size(); ++i)
			{
				if (!g_users[i]->IsMe)
				{
					s3eInetAddress* addr = &g_users[i]->Address;
					bytesSent += s3eSocketSendTo(g_pSock, g_szSendBuffer, offset, 0, addr);
				}
			}
		}
		else
		{
			bytesSent += s3eSocketSendTo(g_pSock, g_szSendBuffer, offset, 0, &g_addr);
			g_lastSend = s3eTimerGetMs();
		}

		if (bytesSent > 0)
		{
			g_iSend += bytesSent;
		}
		else
		{
			IwGetNotificationHandler()->PushNotification((int)this, "Failed to Send", 5000);
		}

		s3eSocketError err = s3eSocketGetError();
		s3eDeviceYield(0);
	}
	else
	{
		char szRequest[500];
		strcpy(szRequest, "http://mobilesrc.appspot.com/");
		strcat(szRequest, szQueryString);

		IwGetHTTPQueue()->Post(szRequest, g_szBuffer, g_bufferOffset, pData, callback, callbackError, timeout);
	}
}

void CIwMultiplayerHandler::Get(MultiplayerGameMessage message, char* szQueryString, void* pData, pCallback callback, pCallbackError callbackError, int timeout = 5000)
{
	if (g_bSocketMode)
	{
		Post(message, szQueryString, NULL, 0, pData, callback, callbackError, timeout);
	}
	else
	{
		char szRequest[500];
		strcpy(szRequest, "http://mobilesrc.appspot.com/");
		strcat(szRequest, szQueryString);

		IwGetHTTPQueue()->Get(szRequest, pData, callback, callbackError, timeout);
	}
}

void CIwMultiplayerHandler::HandleDisconnect(char* szReason)
{
	// Don't close since we are UDP
	//s3eSocketClose(g_pSock);
	s3eSocketClose(g_pSock);
	g_pSock = NULL;

	EndGame();

	if (g_pDisconnectCallback)
	{
		g_pDisconnectCallback(true, szReason, g_disconnectUserData);
	}
}

void CIwMultiplayerHandler::Update()
{
	if (g_bSocketMode)
	{
		// handle sockets here
		// receive messages of the format
		// messagetype|messageLength (int)|package
		if (NULL != g_pSock)
		{
			uint64 timer = s3eTimerGetMs();
			uint64 updateDiff = (timer - g_lastSuccess);
			uint64 sendDiff = (timer - g_lastSend);

			if (MPM_INIT != g_mode)
			{
				if (g_bNeedFlush)
				{
					Flush();
				}
				if (updateDiff > 5000/* && g_users.size() > 1*/)
				{
					IwGetNotificationHandler()->PushNotification((int)this, "... Connection Lag Detected ...", 5000);
				}
				else
				{
					IwGetNotificationHandler()->ClearNotification((int)this);
				}
				if (sendDiff > 3500)
				{
					Post(MPGM_NOP, "", "", 0, this, NULL, NULL, 1500);
				}
			}

			s3eInetAddress sender;
			int32 bytesRead = s3eSocketRecvFrom(g_pSock, g_szRecvBuffer, sizeof(g_szRecvBuffer), 0, &sender);
			s3eSocketError err = s3eSocketGetError();
			if (bytesRead == -1)
			{
			}
			else
			{
				if (bytesRead > 0)
				{
					g_iRecv += bytesRead;
				}

				uint32 timeStamp = (uint32)g_szRecvBuffer[3] | (((uint32)g_szRecvBuffer[2]) << 8) | (((uint32)g_szRecvBuffer[1]) << 16) | (((uint32)g_szRecvBuffer[0]) << 24);
				MultiplayerGameMessage messageType = (MultiplayerGameMessage)g_szRecvBuffer[4];

				//uint32* pLastPacketTimeStamp = &g_lastPacketTimeStamp;

				//for (int i = 0; i < g_users.size(); ++i)
				//{
				//	if (!g_users[i]->IsMe)
				//	{
				//		if (g_users[i]->Address.m_Port == sender.m_Port && g_users[i]->Address.m_IPAddress == sender.m_IPAddress)
				//		{
				//			pLastPacketTimeStamp = &g_users[i]->LastPacket;
				//		}
				//	}
				//}

				if (timeStamp > g_lastPacketTimeStamp)
				{
					g_lastPacketTimeStamp = timeStamp;

					if (MPM_IN_GAME == g_mode)
					{
						if (messageType != MPGM_Disconnect && messageType != MPGM_PushStateResponse && messageType != MPGM_PushState && messageType != MPGM_NOP)
						{
							IwGetNotificationHandler()->PushNotification((int)this, "In Game: got bad message", 5000);
							return;
						}
					}

					if (bytesRead < sizeof(g_szRecvBuffer))
					{
						g_szRecvBuffer[bytesRead] = 0;
					}

					char* pDataGram = g_szRecvBuffer + 5;
					int dataGramLength = bytesRead - 5;

					switch (messageType)
					{
						case MPGM_Disconnect :
							{
								HandleDisconnect(pDataGram);
							}
							break;
						case MPGM_GetGameDetailsResponse :
							{
								GotGameDetails(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_PushState :
						case MPGM_PushStateResponse :
							{
								GotGameState(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_CreateResponse :
							{
								GotCreateGame(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_ListResponse :
							{
								GotListGame(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_JoinResponse :
							{
								GotJoinGame(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_SetReadyResponse :
							{
								GotSetReady(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_StartResponse :
							{
								GotStartGame(this, NULL, pDataGram, dataGramLength);
							}
							break;
						case MPGM_NOP :
							{
								g_lastSuccess = s3eTimerGetMs();
							}
							break;
					}
				}
				else
				{
					IwGetNotificationHandler()->PushNotification((int)this, "Got old data", 5000);
					s3eDeviceYield(0);
				}
			}
		}
	}
	else if (g_mode == MPM_IN_GAME)
	{
		uint64 timer = s3eTimerGetMs();
		uint64 updateDiff = (timer - g_lastSuccess);

		if (updateDiff > 5000 && g_users.size() > 1)
		{
			IwGetNotificationHandler()->PushNotification((int)this, "... Connection Lag Detected ...", 5000);
		}
		else
		{
			IwGetNotificationHandler()->ClearNotification((int)this);
		}
	}
	else if (g_mode != MPM_IN_GAME && g_mode != MPM_INIT)
	{
		uint64 timer = s3eTimerGetMs();
		uint64 updateDiff = (timer - g_lastUpdate);

		if (!g_bInRequest && updateDiff > 2000)
		{
			char szRequest[500];
			char szId[150];
			GetDeviceId(szId);
			strcpy(szRequest, "");

			pCallback callback = NULL;

			switch (g_mode)
			{
				case MPM_M_WAITING_FOR_READY :
				{
					// Poll to see if all users are ready
					// We don't need the list of users here either since we already confirmed them all
					sprintf(szRequest, "http://mobilesrc.appspot.com/getdetails?&game=%s&id=%s", g_szGameId, szId);
					callback = GotGameDetails;
				}
				break;
				case MPM_S_WAITING_FOR_CONFIRM :
				{
					// Poll to see if we have been accepted
					// We don't want to see the list of users yet
					sprintf(szRequest, "http://mobilesrc.appspot.com/joincheck?&game=%s&id=%s", g_szGameId, szId);
					callback = GotJoinState;
				}
				break;
				case MPM_S_WAITING_FOR_READY :
				{
					// Poll to see if the game is ready to begin
					sprintf(szRequest, "http://mobilesrc.appspot.com/getdetails?&game=%s&id=%s", g_szGameId, szId);
					callback = GotGameDetails;
				}
				break;
				case MPM_S_WAITING_FOR_START :
				{
					// Wait to get the ok that everyone is ready
					sprintf(szRequest, "http://mobilesrc.appspot.com/slavepull?&game=%s&id=%s", g_szGameId, szId);
					callback = GotGameState;
				}
				break;
			}

			if (strlen(szRequest) > 0)
			{
				g_bInRequest = true;

				EscapeUrl(szRequest);
				IwGetHTTPQueue()->Get(szRequest, this, callback, GotError, 4000);
				g_lastUpdate = timer;
			}
		}
	}
	if (!g_bInRequest)
	{
		g_szBuffer[0] = 0;
		g_bufferOffset = 1;
	}
}

void CIwMultiplayerHandler::GotGameDetails(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	// <Game><Ready>False</Ready><Users><User><User>nclark-dev</User><Pending>True</Pending><Ready>True</Ready><Latitude>None</Latitude><Longitude>None</Longitude></User></Users></Game>
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	pThisMP->g_bInRequest = false;
	pThisMP->g_lastSuccess = s3eTimerGetMs();


	char szId[150];
	pThisMP->GetDeviceId(szId);

	CIwArray<User*> deleteUsers;

	for (uint32 i = 0; i < pThisMP->g_users.size(); i++)
	{
		deleteUsers.append(pThisMP->g_users[i]);
	}

	TiXmlDocument doc;
	doc.Parse(Result);

	TiXmlElement* gameNode = doc.RootElement();

	pThisMP->g_bAllUsersReady = false;
	if (gameNode)
	{
		CIwArray<User*> inOrderUsers;

		if (gameNode->FirstChildElement("Ready"))
		{
			pThisMP->g_bAllUsersReady = true;

			const char* szReady = gameNode->FirstChildElement("Ready")->GetText();
			const TiXmlElement* mapNode = gameNode->FirstChildElement("MapData");
			const TiXmlNode* usersNode = gameNode->FirstChild("Users");

			if (!pThisMP->g_bReceivedMapData)
			{
				pThisMP->g_bReceivedMapData = true;

				const char* szMap = mapNode->GetText();
				CIwArray<s3eLocation> loadedPoints;

                Utils::SaveMapData("maps/mp.map", szMap);

                float zoom;
                if (Utils::LoadMap(szMap, &loadedPoints, &zoom))
                {
					Region r;

					Utils::LoadRegion(r, &loadedPoints);
					pThisMP->g_pHandler->SetBoundingRegion("maps/mp.map", &r);
					pThisMP->g_pHandler->SetGameState(GPS_GameState_SelectGame, AnimDir_Left);
                }
			}
			
			pThisMP->g_masterDimensions.x = atoi(gameNode->FirstChildElement("Width")->GetText());
			pThisMP->g_masterDimensions.y = atoi(gameNode->FirstChildElement("Height")->GetText());

			
			pThisMP->g_scaleRatio.x = MAX(0.25f, ((float)pThisMP->g_masterDimensions.x) / Iw2DGetSurfaceWidth());
			pThisMP->g_scaleRatio.y = MAX(0.25f, ((float)pThisMP->g_masterDimensions.y) / Iw2DGetSurfaceHeight());

			bool isReady = (0 != strstr(szReady, "False"));

			pThisMP->g_bGameReady = isReady;

			if (isReady && !pThisMP->IsMaster())
			{
				pThisMP->SetMode(MPM_S_WAITING_FOR_START);
			}

			if (usersNode)
			{
				const TiXmlNode* userNode = usersNode->FirstChild("User");

				uint uiIndex = 0;
				while (userNode)
				{
					const char* szDevice = userNode->FirstChildElement("Device")->GetText();
					const char* szUser = userNode->FirstChildElement("User")->GetText();
					const char* szAccepted = userNode->FirstChildElement("Accepted")->GetText();
					const char* szReady = userNode->FirstChildElement("Ready")->GetText();
					const char* szLatitude = userNode->FirstChildElement("Latitude")->GetText();
					const char* szLongitude = userNode->FirstChildElement("Longitude")->GetText();
					const char* szAddress = userNode->FirstChildElement("Address")->GetText();
					const char* szLocalAddress = userNode->FirstChildElement("LocalAddress")->GetText();
					const char* szPort = userNode->FirstChildElement("Port")->GetText();

					User* pUser = NULL;
					bool createdNew = false;

					for (uint32 i = 0; i < deleteUsers.size(); i++)
					{
						if (0 == stricmp(szDevice, deleteUsers[i]->szDevice))
						{
							pUser = deleteUsers[i];
							bool success = deleteUsers.find_and_remove(deleteUsers[i]);

							if (!success)
							{
								s3eDeviceYield(0);
							}
							break;
						}
					}
					
					if (NULL == pUser)
					{
						pUser = new User;
						createdNew = true;
					}
					strncpy(pUser->szDevice, szDevice, sizeof(pUser->szDevice));
					strncpy(pUser->szName, szUser, sizeof(pUser->szDevice));

					pUser->Score = 0;
					pUser->Accepted = (NULL != strstr(szAccepted, "True"));
					pUser->Ready = (NULL != strstr(szReady, "True"));

					if (pUser->Accepted && !pUser->Ready)
					{
						pThisMP->g_bAllUsersReady = false;
					}

					pUser->Latitude = atof(szLatitude);
					pUser->Longitude = atof(szLongitude);

					memset(&pUser->Address, 0, sizeof(pUser->Address));
					s3eInetAton(&pUser->Address.m_IPAddress, szAddress);
					s3eInetNtoa(pUser->Address.m_IPAddress, pUser->Address.m_String, 128);
					pUser->Address.m_Port = s3eInetHtons(atoi(szPort));

					if (s3eDebugIsDebuggerPresent())
					{
						//if (strstr(pUser->Address.m_String, "192.168."))
						//{
						//	pUser->Address.m_Local = true;
						//	s3eInetAton(&pUser->Address.m_IPAddress, szLocalAddress);
						//	s3eInetNtoa(pUser->Address.m_IPAddress, pUser->Address.m_LocalAddress, 128);
						//}
					}

					pUser->LastPacket = 0;
					pUser->IsMe = (0 == stricmp(szId, pUser->szDevice));
					pUser->uiIndex = uiIndex;

					uiIndex++;

					inOrderUsers.append(pUser);
					userNode = userNode->NextSibling("User");
				}
			}
		}
		else
		{
			s3eDeviceYield(0);
		}
		pThisMP->g_users.clear();

		for (uint32 i = 0; i < inOrderUsers.size(); ++i)
		{
			pThisMP->g_users.append(inOrderUsers[i]);
		}

		for (uint32 i = 0; i < deleteUsers.size(); i++)
		{
			pThisMP->g_users.find_and_remove(deleteUsers[i]);

			if (deleteUsers[i] != pThisMP->g_pMe)
			{
				delete deleteUsers[i];
			}
		}
	}
}

void CIwMultiplayerHandler::GotGameState(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	// Binary data -- if 0, we don't have a state yet
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	pThisMP->g_bInRequest = false;
	pThisMP->g_lastSuccess = s3eTimerGetMs();

	if (!pThisMP->g_bSocketMode)
	{
		// reset our push state
		g_szBuffer[0] = 0;
		g_bufferOffset = 1;
	}

	//const char* szStart = "mobilesrc"
	const char* szStart = "";

	if (ResultLen > strlen(szStart))
	{
		// We have a game state. Parse it and farm it out
		// We know our message length, the first byte should be the number of packages
		// Each package contains a set of messages
		const char* szReader = Result;
		const char* szEnd = Result+ResultLen;

		if (szReader == strstr(szReader, szStart))
		{
			if (pThisMP->g_mode == MPM_S_WAITING_FOR_START)
			{
				pThisMP->SetMode(MPM_IN_GAME);
			}
			szReader += strlen(szStart);

			//char packageCount = szReader[0];
			//szReader++;
			char packageCount = 1;

			for (int pcnt = 0; pcnt < packageCount; ++pcnt)
			{
				uint8 messageCount = szReader[0];
				szReader++;

				// todo -- add some checks for buffer overruns
				for (uint32 i = 0; i < messageCount; ++i)
				{
					// Overrun check!
					if ((szEnd - szReader) >= 2)
					{
						// the first byte is the identifier
						uint8 identifier = szReader[0];
						szReader++;

						// the second byte is the message length.
						uint8 messageLength = szReader[0];
						szReader++;
						
						// Overrun check!
						if ((szEnd - szReader) >= messageLength)
						{
							// the remaining bytes are message data
							for (uint32 j = 0; j < pThisMP->g_Messages.size(); j++)
							{
								if (pThisMP->g_Messages[j].identifier == identifier)
								{
									pThisMP->g_Messages[j].pCallback(szReader, messageLength, pThisMP->g_Messages[j].userData);
									break;
								}
							}
							szReader += messageLength;
						}
						else
						{
							IwGetNotificationHandler()->PushNotification((int)pThisMP, "break1", 5000);
							IwTrace("FF", ("break1"));
							break;
						}
					}
					else
					{
						IwGetNotificationHandler()->PushNotification((int)pThisMP, "break2", 5000);
						IwTrace("FF", ("break2"));
						break;
					}
				}
			}
		}
	}
}

void CIwMultiplayerHandler::GotJoinState(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	// <Response>bool</Response>
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;

	if (ResultLen > 0)
	{
		TiXmlDocument doc;
		doc.Parse(Result);

		TiXmlElement* gameNode = doc.RootElement();

		if (gameNode)
		{
			const char* szAccepted = gameNode->GetText();

			bool isAccepted = false;
			if (strstr(szAccepted, "True"))
			{
				pThisMP->SetMode(MPM_S_WAITING_FOR_READY);
				isAccepted = true;
			}
			pThisMP->g_bAccepted = isAccepted;
		}
	}

	pThisMP->g_bInRequest = false;
}

void CIwMultiplayerHandler::GotError(void * pThis)
{
	IwGetNotificationHandler()->PushNotification(0, "GotError", 5000);
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;

	pThisMP->g_bInRequest = false;
}

CIwArray<CIwMultiplayerHandler::User*> CIwMultiplayerHandler::ListUsers()
{
	return g_users;
}

CIwMultiplayerHandler::User* CIwMultiplayerHandler::GetMe()
{
	for (int i = 0; i < g_users.size(); ++i)
	{
		if (g_users[i]->IsMe)
		{
			return g_users[i];
		}
	}
	return NULL;
}

void CIwMultiplayerHandler::AcceptUser(CIwMultiplayerHandler::User* pUser)
{
	char szRequest[1500];
	sprintf(szRequest, "joindecide?&game=%s&id=%s&accept=True", g_szGameId, pUser->szDevice);
	
	Get(MPGM_AcceptUser, szRequest, this, GotGameDetails, GotError);
}

void CIwMultiplayerHandler::RejectUser(CIwMultiplayerHandler::User* pUser)
{
	char szRequest[1500];
	sprintf(szRequest, "joindecide?&game=%s&id=%s&accept=False", g_szGameId, pUser->szDevice);
	
	Get(MPGM_RejectUser, szRequest, this, GotGameDetails, GotError);
}

void CIwMultiplayerHandler::StartGame()
{
	if (strlen(g_szGameId) > 0)
	{
		if (IsMaster())
		{
			char szRequest[1500];
			sprintf(szRequest, "start?&game=%s", g_szGameId);
			
			EscapeUrl(szRequest);

			Get(MPGM_Start, szRequest, this, &CIwMultiplayerHandler::GotStartGame, &CIwMultiplayerHandler::StartGameWebServiceError);
		}
		else
		{
			SetReady();
		}
	}
	else
	{
		SetMode(MPM_INIT);
	}
}

void CIwMultiplayerHandler::GotStartGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	pThisMP->SetMode(MPM_IN_GAME);

	// We should throw an event here...or just poll perhaps
}

void CIwMultiplayerHandler::StartGameWebServiceError(void* pThis)
{
	IwGetNotificationHandler()->PushNotification(0, "StartGameWebServiceError", 5000);
}

void CIwMultiplayerHandler::SetReady()
{
	if (strlen(g_szGameId) > 0 && !g_bIsReady)
	{
		if (!g_bInRequest)
		{
			g_bInRequest = true;
			char szId[150];
			GetDeviceId(szId);

			char szRequest[1500];
			sprintf(szRequest, "ready?&game=%s&id=%s", g_szGameId, szId);
			
			EscapeUrl(szRequest);

			Get(MPGM_SetReady, szRequest, this, &CIwMultiplayerHandler::GotSetReady, &CIwMultiplayerHandler::SetReadyWebServiceError);
		}
	}
}

void CIwMultiplayerHandler::GotSetReady(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	pThisMP->g_bInRequest = false;
	pThisMP->g_bIsReady = true;

	// We have declared we are ready. We must wait for the start
	if (!pThisMP->IsMaster())
	{
		pThisMP->SetMode(MPM_S_WAITING_FOR_START);
	}
}

void CIwMultiplayerHandler::SetReadyWebServiceError(void* pThis)
{
	IwGetNotificationHandler()->PushNotification(0, "SetReadyWebServiceError", 5000);
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	pThisMP->g_bInRequest = false;
}

void CIwMultiplayerHandler::EndGame()
{
	g_bAccepted = false;
	if (strlen(g_szGameId) > 0 && IsMaster())
	{
		char szRequest[1500];
		sprintf(szRequest, "end?&game=%s", g_szGameId);
		
		EscapeUrl(szRequest);

		Get(MPGM_EndGame, szRequest, this, &CIwMultiplayerHandler::GotEndGame, &CIwMultiplayerHandler::EndGameWebServiceError);
		SetMode(MPM_GAME_OVER);
	}
	else
	{
		SetMode(MPM_INIT);
	}

	g_messageIdentifier = 0;
	if (g_pSock)
	{
		s3eSocketClose(g_pSock);
		g_pSock = NULL;
	}

	for (uint32 i = 0; i < g_users.size(); ++i)
	{
		if (g_users[i] != g_pMe)
		{
			delete g_users[i];
		}
	}
	g_users.clear();

	strcpy(g_szGameId, "");
}

void CIwMultiplayerHandler::GotEndGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	pThisMP->SetMode(MPM_INIT);
}

void CIwMultiplayerHandler::EndGameWebServiceError(void* pThis)
{
	IwGetNotificationHandler()->PushNotification(0, "EndGameWebServiceError", 5000);
}

void CIwMultiplayerHandler::StartMultiplayerGame(ConnectionChangedCallback pCallback, void* userData)
{
	g_bReceivedMapData = true;
	g_bMaster = true;
	g_startUserData = userData;
	g_pStartCallback = pCallback;

	s3eLocation loc;
	if (S3E_RESULT_ERROR == s3eLocationGet(&loc))
	{
		s3eLocationStart();
		s3eLocationGet(&loc);
		s3eLocationStop();
	}

	char szId[150];
	GetDeviceId(szId);
	const char* szName = s3eDeviceGetString(S3E_DEVICE_NAME);
	
	char szRequest[1500];
	sprintf(szRequest, "create?&id=%s&user=%s&latitude=%f&longitude=%f&width=%d&height=%d&gameId=%d", szId, szName, loc.m_Latitude, loc.m_Longitude, Iw2DGetSurfaceWidth(), Iw2DGetSurfaceHeight(), RUNAWAYGAME_ID);
	
	EscapeUrl(szRequest);

    char* szMap = g_pHandler->GetLoadedRegion();
    char* szData = "-tilt-";
    if (0 != stricmp(szMap, "-tilt-"))
    {
        szData = Utils::GetMapData(szMap);
    }

	g_messageIdentifier = 0;
	SetMode(MPM_M_CREATING);
	Post(MPGM_Create, szRequest, szData, strlen(szData), this, &CIwMultiplayerHandler::GotCreateGame, &CIwMultiplayerHandler::CreateGameWebServiceError);
}

void CIwMultiplayerHandler::GotCreateGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	if (ResultLen > 0 && ResultLen < 50)
	{
		pThisMP->SetGameId(Result);
		if (pThisMP->g_pStartCallback)
		{
			pThisMP->g_pStartCallback(true, "Game Registered. Waiting for users...", pThisMP->g_startUserData);
		}
		pThisMP->SetMode(MPM_M_WAITING_FOR_READY);
	}
	else
	{
		if (pThisMP->g_pStartCallback)
		{
			pThisMP->g_pStartCallback(false, Result, pThisMP->g_startUserData);
		}
		pThisMP->SetMode(MPM_INIT);
	}
}

void CIwMultiplayerHandler::CreateGameWebServiceError(void* pThis)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	IwGetNotificationHandler()->PushNotification(0, "CreateGameWebServiceError", 5000);
	if (pThisMP->g_pStartCallback)
	{
		pThisMP->g_pStartCallback(false, "Error", pThisMP->g_startUserData);
	}
}

void CIwMultiplayerHandler::JoinMultiplayerGame(ConnectionChangedCallback pCallback, void* userData)
{
	g_bIsReady = false;
	g_bReceivedMapData = false;
	g_bMaster = false;
	g_joinUserData = userData;
	g_pJoinCallback = pCallback;

	s3eLocation loc;
	if (S3E_RESULT_ERROR == s3eLocationGet(&loc))
	{
		s3eLocationStart();
		s3eLocationGet(&loc);
		s3eLocationStop();
	}

	char szId[150];
	GetDeviceId(szId);
	const char* szName = s3eDeviceGetString(S3E_DEVICE_NAME);

	char szRequest[1500];
	sprintf(szRequest, "list?&id=%s&user=%s&latitude=%f&longitude=%f&gameId=%d", szId, szName, loc.m_Latitude, loc.m_Longitude, RUNAWAYGAME_ID);
	
	EscapeUrl(szRequest);

	g_messageIdentifier = 0;
	SetMode(MPM_S_JOINING);
	Get(MPGM_List, szRequest, this, &CIwMultiplayerHandler::GotListGame, &CIwMultiplayerHandler::ListGameWebServiceError);
}

void CIwMultiplayerHandler::SetGameId(const char* szGameId)
{
	int32 seed = 0;

	for (uint32 i = 0; i < strlen(szGameId); ++i)
	{
		seed += szGameId[i];
	}
	IwRandSeed(seed);

	// Clear out notifications
	g_Messages.clear();
	g_identifier = 0;

	strncpy(g_szGameId, szGameId, sizeof(g_szGameId));
}

void CIwMultiplayerHandler::GotListGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;

	bool success = false;
	TiXmlDocument doc;

	doc.Parse(Result);

	TiXmlElement* rootElem = doc.RootElement();

	if (rootElem)
	{
		const TiXmlNode* collectionNode = rootElem->FirstChild("GameCollection");

		if (rootElem)
		{
			const TiXmlNode* gameNode = rootElem->FirstChild("Game");
			const TiXmlNode* lastNode = NULL;

			while (gameNode)
			{
				lastNode = gameNode;
				gameNode = gameNode->NextSibling("Game");
			}

			// TODO: actually show the games
			if (lastNode)
			{
				const char* szGameId = lastNode->FirstChildElement("Id")->GetText();
				const char* szGameName = lastNode->FirstChildElement("User")->GetText();

				pThisMP->SetGameId(szGameId);
				
				success = true;

				if (pThisMP->g_pJoinCallback)
				{
					pThisMP->g_pJoinCallback(true, "Found Game. Connecting...", pThisMP->g_joinUserData);
				}

				char szId[150];
				pThisMP->GetDeviceId(szId);
				const char* szName = s3eDeviceGetString(S3E_DEVICE_NAME);

				s3eLocation loc;
				if (S3E_RESULT_ERROR == s3eLocationGet(&loc))
				{
					s3eLocationStart();
					s3eLocationGet(&loc);
					s3eLocationStop();
				}

				char szRequest[1500];
				sprintf(szRequest, "join?&game=%s&id=%s&user=%s&latitude=%f&longitude=%f", szGameId, szId, szName, loc.m_Latitude, loc.m_Longitude);
				
				EscapeUrl(szRequest);

				pThisMP->Get(MPGM_Join, szRequest, pThisMP, &CIwMultiplayerHandler::GotJoinGame, &CIwMultiplayerHandler::JoinGameWebServiceError);
			}
		}
	}
	if (!success)
	{
		if (pThisMP->g_pJoinCallback)
		{
			pThisMP->g_pJoinCallback(false, "No games found...", pThisMP->g_joinUserData);
		}
	}
}

void CIwMultiplayerHandler::ListGameWebServiceError(void* pThis)
{
	IwGetNotificationHandler()->PushNotification(0, "ListGameWebServiceError", 5000);
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	if (pThisMP->g_pJoinCallback)
	{
		pThisMP->g_pJoinCallback(false, "Error Finding Games...", pThisMP->g_joinUserData);
	}
}

void CIwMultiplayerHandler::GotJoinGame(void * pThis, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	if (ResultLen == 0)
	{
		pThisMP->SetMode(MPM_S_WAITING_FOR_CONFIRM);
		pThisMP->g_pJoinCallback(true, "Awaiting Confirmation...", pThisMP->g_joinUserData);
	}
	else
	{
		pThisMP->SetMode(MPM_INIT);
		pThisMP->g_pJoinCallback(false, "Error Joining Game...", pThisMP->g_joinUserData);
	}
}

void CIwMultiplayerHandler::JoinGameWebServiceError(void* pThis)
{
	IwGetNotificationHandler()->PushNotification(0, "JoinGameWebServiceError", 5000);
	CIwMultiplayerHandler* pThisMP = (CIwMultiplayerHandler*)pThis;
	if (pThisMP->g_pJoinCallback)
	{
		pThisMP->g_pJoinCallback(false, "Error Finding Games...", pThisMP->g_joinUserData);
	}
}