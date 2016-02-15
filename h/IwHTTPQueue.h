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

#ifndef IW_HTTPQUEUE_H
#define IW_HTTPQUEUE_H

#include "IwSingleton.h"
#include "IwHTTP.h"
#include "IwArray.h"

typedef void (*pCallback)(void * Argument, const char* szContentType, const char * Result, uint32 ResultLen);
typedef void (*pCallbackError)(void * Argument);

class CIwHTTPQueue
{
public:
	CIwHTTPQueue();
	~CIwHTTPQueue();
	void Get(const char* pURL, void* pArgument, pCallback Callback, pCallbackError CallbackError, int timeout = 15000);
	void Post(const char* pURL, char* szBuffer, int bufferLen, void* pArgument, pCallback Callback, pCallbackError CallbackError, int timeout = 15000);
	void GetFirst(const char* pURL, void* pArgument, pCallback Callback, pCallbackError CallbackError, int timeout = 15000);
	void CancelArgument(void * pArgument);
	void Update();

	void SetMaxFileSize(int32 maxsize){ m_MaxFileSize = maxsize; }
	bool IsSizeAllowed(); // Abandon download if file is too big

private:
	void Cancel();

	struct Request
	{
		const char* pURL;
		char * szBuffer;
		int bufferLen;
		bool usePost;
		void * pArgument;
		pCallback Callback;
		pCallbackError CallbackError;
		int timeout;
	};
	CIwArray<Request> m_Request;
	CIwHTTP m_HTTP;

	int32	m_MaxFileSize;
	int     m_timeout;
	uint64  m_requestStart;
};

IW_SINGLETON_EXTERN(HTTPQueue);

#endif
