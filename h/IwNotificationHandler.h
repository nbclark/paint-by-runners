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

#ifndef IW_NOTIFHANDLER_H
#define IW_NOTIFHANDLER_H

#include "IwSingleton.h"
#include "s3e.h"
#include "s3eSocket.h"
#include "IwArray.h"
#include "IwGx.h"
#include "IGameHandler.h"

#define NotificationHeight 32

class CIwNotificationHandler
{
public:

	CIwNotificationHandler();
	~CIwNotificationHandler();

	void Update();
	void Render();
	
	void PushNotification(int identifier, char* szMessage, uint64 duration);
	void ClearNotification(int identifier);

private :

	struct Notification
	{
		int identifier;
		char szMessage[200];
		uint64 expiration;
		int animation;
	};

	CIwArray<Notification*> g_notifications;
	CIwGxFont* g_pFont;
};

IW_SINGLETON_EXTERN(NotificationHandler);

#endif
