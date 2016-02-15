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
#include "IwNotificationHandler.h"
#include "IwUtil.h"
#include "Iw2D.h"
#include "IwDebug.h"
#include "IwRuntime.h"
#include "IwHttpQueue.h"
#include "s3eLocation.h"
#include "tinyxml.h"
#include "Utils.h"

// http://mobilesrc.appspot.com

IW_SINGLETON_INSTANTIATE(NotificationHandler);

CIwNotificationHandler::CIwNotificationHandler()
{
	IW_SINGLETON_SET(NotificationHandler);
	g_pFont = (CIwGxFont*)IwGetResManager()->GetResNamed("font_medium", "CIwGxFont");
}

CIwNotificationHandler::~CIwNotificationHandler()
{
}

void CIwNotificationHandler::Render()
{
	int startHeight = Iw2DGetSurfaceHeight() - ((g_notifications.size()-1) * (NotificationHeight + 1));
	int width = Iw2DGetSurfaceWidth();

	IwGxLightingOn();
	IwGxFontSetFont(g_pFont);
	IwGxFontSetCol(0xffffffff);

	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
	
	//Iw2DSetColour(0);

	CIwColour oldCol = Iw2DGetColour();
	CIwColour fill = { 0x00, 0x20, 0x88, 0xff };
	CIwColour border = { 0, 0, 0, 0xf0 };

	for (int i = g_notifications.size()-1; i >= 0; i--)
	{
		CIwRect rect(0, startHeight - g_notifications[i]->animation, width, NotificationHeight);
		
		Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
		Iw2DSetColour(border);
		Iw2DFillRect(CIwSVec2(rect.x, rect.y), CIwSVec2(rect.w, rect.h));
		Iw2DSetColour(fill);
		Iw2DFillRect(CIwSVec2(rect.x + 2, rect.y + 2), CIwSVec2(rect.w-4, rect.h-4));
		Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);

		IwGxFontSetRect(rect);

		IwGxFontDrawText(g_notifications[i]->szMessage);

		// startHeight - (animation)
		startHeight += NotificationHeight + 1;
	}
	Iw2DSetColour(oldCol);
	IwGxLightingOff();
	Iw2DFinishDrawing();
}

void CIwNotificationHandler::Update()
{
	CIwArray<Notification*> clear;

	uint64 time = s3eTimerGetMs();
	for (uint32 i = 0; i < g_notifications.size(); ++i)
	{
		if (g_notifications[i]->expiration < time)
		{
			clear.append(g_notifications[i]);
		}
		else
		{
			if (g_notifications[i]->animation < NotificationHeight)
			{
				g_notifications[i]->animation += 4;
			}
		}
	}
	for (uint32 i = 0; i < clear.size(); ++i)
	{
		g_notifications.find_and_remove(clear[i]);
		delete clear[i];
	}
}
	
void CIwNotificationHandler::PushNotification(int identifier, char* szMessage, uint64 duration)
{
	uint64 expiration = s3eTimerGetMs() + duration;

	Notification* pNot = NULL;

	for (uint32 i = 0; i < g_notifications.size(); ++i)
	{
		if (g_notifications[i]->identifier == identifier)
		{
			pNot = g_notifications[i];
			break;
		}
	}

	if (!pNot)
	{
		pNot = new Notification;
		pNot->animation = 0;
		g_notifications.append(pNot);
	}

	pNot->identifier = identifier;
	strncpy(pNot->szMessage, szMessage, sizeof(pNot->szMessage));
	pNot->expiration = expiration;
}

void CIwNotificationHandler::ClearNotification(int identifier)
{
	for (uint32 i = 0; i < g_notifications.size(); ++i)
	{
		if (g_notifications[i]->identifier == identifier)
		{
			Notification* pNot = g_notifications[i];
			g_notifications.find_and_remove(g_notifications[i]);
			delete pNot;

			break;
		}
	}
}