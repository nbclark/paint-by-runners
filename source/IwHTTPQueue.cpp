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
#include "IwHTTPQueue.h"
#include "IwUtil.h"
#include "IwDebug.h"
#include "IwRuntime.h"

char* g_Result = NULL;
char* g_szContentType[100];
uint32 g_ResultLen;
bool g_GotResult = false;

IW_SINGLETON_INSTANTIATE(HTTPQueue);

CIwHTTPQueue::CIwHTTPQueue()
: m_MaxFileSize(-1)
{
	IW_SINGLETON_SET(HTTPQueue);
	m_timeout = 30000;
}

CIwHTTPQueue::~CIwHTTPQueue()
{
	if (g_Result)
	{
		s3eFree(g_Result);
	}

	for (int u=m_Request.size()-1;u>=0;u--)
	{
		delete[] (char*)m_Request[u].pURL;
	}
	
	m_Request.clear();
}	

static int32 GotData(void* object, void*)
{
	CIwHTTP * theHttpObject = (CIwHTTP*)object;
    // This is the callback indicating that a ReadContent call has
    // completed.  Either we've finished, or a bigger buffer is
    // needed.  If the correct ammount of data was supplied initially,
    // then this will only be called once. However, it may well be
    // called several times when using chunked encoding.

    // Firstly see if there's an error condition.
    if (theHttpObject->GetStatus() == S3E_RESULT_ERROR)
    {
		// Free data now
        s3eFree(g_Result);
		g_Result = 0;
		g_GotResult = true;
    }
    else if (theHttpObject->ContentReceived() != theHttpObject->ContentLength())
    {
        // We have some data but not all of it. We need more space.
        uint32 oldLen = g_ResultLen;
        // If iwhttp has a guess how big the next bit of data is (this
        // basically means chunked encoding is being used), allocate
        // that much space. Otherwise guess.
        if (g_ResultLen < theHttpObject->ContentExpected())
            g_ResultLen = theHttpObject->ContentExpected();
        else
            g_ResultLen += 1024;

		if(!IwGetHTTPQueue()->IsSizeAllowed())
			return 0;

        // Allocate some more space and fetch the data.
        g_Result = (char*)s3eRealloc(g_Result, g_ResultLen);
        theHttpObject->ReadContent(&g_Result[oldLen], g_ResultLen - oldLen, GotData);
    }
    else
    {
		uint32 code = theHttpObject->GetResponseCode();
		g_ResultLen = theHttpObject->ContentReceived();
		g_GotResult = true;

		_STL::string imageType;
		theHttpObject->GetHeader("content-type", imageType);

		const char* szImageType = imageType.c_str();
		int len = strlen(szImageType);

		char* szImageTypeCpy = new char[len+1];
		strcpy(szImageTypeCpy, szImageType);

		for (int i = 0; i < len;++i)
		{
			szImageTypeCpy[i] = tolower(szImageTypeCpy[i]);
		}

		strcpy((char*)g_szContentType, szImageTypeCpy);
		
		delete[] szImageTypeCpy;
    }
    return 0;
}

//-----------------------------------------------------------------------------
// Called when the response headers have been received
//-----------------------------------------------------------------------------
static int32 GotHeaders(void * object, void*)
{
	CIwHTTP * theHttpObject = (CIwHTTP*)object;

    if (theHttpObject->GetStatus() == S3E_RESULT_ERROR)
    {
        // Something has gone wrong
		g_Result = 0;
		g_GotResult = true;
    }
    else
    {
        // Depending on how the server is communicating the content
        // length, we may actually know the length of the content, or
        // we may know the length of the first part of it, or we may
        // know nothing. ContentExpected always returns the smallest
        // possible size of the content, so allocate that much space
        // for now if it's non-zero. If it is of zero size, the server
        // has given no indication, so we need to guess. We'll guess at 1k.
		int contentExpected = theHttpObject->ContentExpected();
		g_ResultLen = contentExpected;
        if (!g_ResultLen)
        {
            g_ResultLen = 1024;
        }
		if(!IwGetHTTPQueue()->IsSizeAllowed())
			return 0;

        g_Result = (char*)s3eMalloc(g_ResultLen + 1);
        g_Result[contentExpected] = 0;

        theHttpObject->ReadContent(g_Result, contentExpected, GotData, NULL);
    }
    return 0;
}

// Add request the back of the queue
void CIwHTTPQueue::Get(const char* pURL, void* pArgument, pCallback Callback, pCallbackError CallbackError, int timeout)
{
	Request addRequest;

	char * newURL = new char [strlen(pURL)+1];
	strcpy(newURL, pURL);

	addRequest.pURL = newURL;
	addRequest.pArgument = pArgument;
	addRequest.Callback = Callback;
	addRequest.CallbackError = CallbackError;
	addRequest.usePost = false;
	addRequest.timeout = timeout;

	m_Request.push_back(addRequest);

	if (m_Request.size() == 1)
	{
		m_HTTP.Cancel();
		m_timeout = timeout;
		m_requestStart = s3eTimerGetMs();
		m_HTTP.Get(pURL, GotHeaders, NULL);
	}
}


// Add request the back of the queue
void CIwHTTPQueue::Post(const char* pURL, char* szBuffer, int bufferLen, void* pArgument, pCallback Callback, pCallbackError CallbackError, int timeout)
{
	Request addRequest;

	char * newURL = new char [strlen(pURL)+1];
	strcpy(newURL, pURL);

	addRequest.pURL = newURL;
	addRequest.pArgument = pArgument;
	addRequest.Callback = Callback;
	addRequest.CallbackError = CallbackError;
	addRequest.usePost = true;
	addRequest.szBuffer = szBuffer;
	addRequest.bufferLen = bufferLen;
	addRequest.timeout = timeout;

	m_Request.push_back(addRequest);

	if (m_Request.size() == 1)
	{
		m_HTTP.Cancel();
		m_timeout = timeout;
		m_requestStart = s3eTimerGetMs();
		m_HTTP.Post(pURL, szBuffer, bufferLen, GotHeaders, NULL);
	}
}

// Add request the front of the queue
void CIwHTTPQueue::GetFirst(const char* pURL, void* pArgument, pCallback Callback, pCallbackError CallbackError, int timeout)
{
	if (m_Request.size() < 2)
	{
		m_timeout = timeout;
		Get(pURL, pArgument, Callback, CallbackError);
		return;
	}

	Request addRequest;

	char * newURL = new char [strlen(pURL)+1];
	strcpy(newURL, pURL);

	addRequest.pURL = newURL;
	addRequest.pArgument = pArgument;
	addRequest.Callback = Callback;
	addRequest.CallbackError = CallbackError;
	addRequest.usePost = false;
	addRequest.timeout = timeout;

	// Insert behind currently pending request
	m_Request.insert_slow(addRequest, 1);
}

void CIwHTTPQueue::CancelArgument(void *pArgument)
{	
	// Cancel any requests with matching argument
	for (int u=m_Request.size()-1;u>=0;u--)
	{
		if (m_Request[u].pArgument == pArgument)
		{
			if (!u) // Request is pending, don't call the callback on completion
				m_Request[u].Callback = 0;
			else
			{
				// Free memory and remove the request
				delete[] (char*)m_Request[u].pURL;
				m_Request.erase(u);
			}
		}
	}
}

void CIwHTTPQueue::Cancel()
{
	m_HTTP.Cancel();
}

bool CIwHTTPQueue::IsSizeAllowed()
{
	if(m_MaxFileSize>=0)
	{
		if((int32)g_ResultLen>m_MaxFileSize)
		{
			if(g_Result)
			{
				s3eFree(g_Result); // Free now to avoid calling file callback
				g_Result = 0;
			}
			g_GotResult = true; // Set true to advance to next file in update call
			return false;
		}
	}
	return true;
}

void CIwHTTPQueue::Update()
{
	if (m_Request.size())
	{
		if (g_GotResult)
		{
			m_HTTP.Cancel();
			g_GotResult = false;

			Request request = m_Request[0];
			m_Request.erase(0);

			if (g_Result)
			{
				// Call the requests callback with the received data
				if (request.Callback)
				{
					request.Callback(request.pArgument, (const char*)g_szContentType, g_Result, g_ResultLen);
				}

				// Free result
				s3eFree(g_Result);
				g_szContentType[0] = 0;
				g_Result = 0;
			}
			else
			{
				if (request.CallbackError)
				{
					request.CallbackError(request.pArgument);
				}
			}

			delete[] (char*)request.pURL;

			// Begin fetching the next request
			if (m_Request.size())
			{
				if (m_Request[0].Callback)
				{
					if (m_Request[0].usePost)
					{
						m_timeout = m_Request[0].timeout;
						m_requestStart = s3eTimerGetMs();
						m_HTTP.Post(m_Request[0].pURL, m_Request[0].szBuffer, m_Request[0].bufferLen, GotHeaders, NULL);
					}
					else
					{
						m_timeout = m_Request[0].timeout;
						m_requestStart = s3eTimerGetMs();
						m_HTTP.Get(m_Request[0].pURL, GotHeaders, NULL);
					}
				}
				else
				{
					g_GotResult = true;
				}
			}
		}
		else
		{
			uint64 now = s3eTimerGetMs();

			if (m_Request[0].timeout != -1 && (now - m_requestStart) > m_Request[0].timeout)
			{
				m_Request[0].timeout = -1;
				g_GotResult = true;
				m_HTTP.Cancel();
				s3eDeviceYield(0);
			}
		}
	}
}
