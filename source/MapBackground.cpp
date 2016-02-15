#include "MapBackground.h"
#include "IwNotificationHandler.h"
#include "IwMultiplayerHandler.h"

CIwTexture* MapBackground::g_pNotFoundTexture = NULL;
CIwTexture* MapBackground::g_pLoadingTexture = NULL;

struct CTouch
{
    int32 x; // position x
    int32 y; // position y
	int32 startX;
	int32 startY;
	bool active; // is touch active (currently down)
    int32 id; // touch's unique identifier
};

#define MAX_TOUCHES 10
CTouch g_Touches[MAX_TOUCHES];
int g_iActiveTouches = 0;

CTouch* GetTouch(int32 id)
{
    CTouch* pInActive = NULL;

    for(uint32 i = 0; i < MAX_TOUCHES; i++)
    {
        if( id == g_Touches[i].id )
		{
            return &g_Touches[i];
		}
        if( !g_Touches[i].active )
		{
            pInActive = &g_Touches[i];
		}
    }

    //Return first inactive touch
    if( pInActive )
    {
        pInActive->id = id;
        return pInActive;
    }

    //No more touches, give up.
    return NULL;
}

void MultiTouchButtonCB(s3ePointerTouchEvent* event)
{
    CTouch* pTouch = GetTouch(event->m_TouchID);
    if (pTouch)
    {
        pTouch->active = event->m_Pressed != 0; 
        pTouch->x = event->m_x;
        pTouch->y = event->m_y;
        pTouch->startX = event->m_x;
        pTouch->startY = event->m_y;
    }
	
	g_iActiveTouches = 0;

	for (int i = 0; i < MAX_TOUCHES; ++i)
	{
		if (g_Touches[i].active)
		{
			g_iActiveTouches++;
		}
	}
}

void MultiTouchMotionCB(s3ePointerTouchMotionEvent* event)
{
    CTouch* pTouch = GetTouch(event->m_TouchID);
    if( pTouch )
    {
        pTouch->x = event->m_x;
        pTouch->y = event->m_y;
    }
}

MapBackground::MapBackground()
{
	g_tempZoom = g_newZoom = g_actualZoom = g_curZoom = 17;// - 1;

	if (NULL == g_pNotFoundTexture)
	{
		g_pNotFoundTexture = new CIwTexture;
		g_pNotFoundTexture->LoadFromFile("notfound.png");
		g_pNotFoundTexture->Upload();
	}
	if (NULL == g_pLoadingTexture)
	{
		g_pLoadingTexture = new CIwTexture;
		g_pLoadingTexture->LoadFromFile("loading.png");
		g_pLoadingTexture->Upload();
	}
	g_bMouseDown = false;
	g_bLocationChanged = false;
	g_dAlpha = 0xFF;

	if (s3ePointerGetInt(S3E_POINTER_MULTI_TOUCH_AVAILABLE))
    {
        //Register for multi touch events on platforms where the functionality is available.
        s3ePointerRegister(S3E_POINTER_TOUCH_EVENT, (s3eCallback)MultiTouchButtonCB, NULL);
        s3ePointerRegister(S3E_POINTER_TOUCH_MOTION_EVENT, (s3eCallback)MultiTouchMotionCB, NULL);
    }

}
MapBackground::~MapBackground()
{
	if (g_pNotFoundTexture)
	{
		delete g_pNotFoundTexture;
	}
	if (g_pLoadingTexture)
	{
		delete g_pLoadingTexture;
	}
}

void MapBackground::Init()
{
	g_pThisAndTile = new ThisAndTile;
	((ThisAndTile*)g_pThisAndTile)->pThis = this;

	gError = S3E_RESULT_SUCCESS;
	gResult = NULL;
	gResultLen = 0;
	pImage = 0;
	g_cursorIter = 1;
	g_bInProgress = false;
	g_bInitialLoad = true;
	g_bIsAnimating = false;
	g_latPerPixel = 1;
	g_lonPerPixel = 1;
	g_iTileCacheCount = 0;

	gLocation.m_Latitude = 47.7710083;
	gLocation.m_Longitude = -122.1588533;

	g_bLocationChanged = true;

	g_viewMatrix.SetIdentity();
	g_viewMatrix.t.x = -0x200;

	g_bScaledMode = false;
	g_bShowCursor = true;

	gResult = NULL;

	CIwImage img;
	img.LoadFromFile("cursor.png");
	bool a = img.UsesAlpha();

	gCursor = Iw2DCreateImage(img);

	gScaler = new CoordinateScaler(Iw2DGetSurfaceWidth(), Iw2DGetSurfaceHeight(), NULL, 0, false);
	gScaledModeScaler = new CoordinateScaler(Iw2DGetSurfaceWidth(), Iw2DGetSurfaceHeight(), NULL, 0, true);
}

//-----------------------------------------------------------------------------
void MapBackground::ShutDown()
{
	EmptyTileList(&gVectorImageUrls, true);

	for (int i = 0; i < g_pTileCache.size(); ++i)
	{
		AbortTile(g_pTileCache[i], true);
	}

	g_pTileCache.clear();

	delete gResult;
	delete gCursor;
	delete gScaler;
	delete gScaledModeScaler;
	delete g_pThisAndTile;
}

void MapBackground::SetAlpha(double dAlpha)
{
	g_dAlpha = dAlpha;
}

void MapBackground::DownloadTiles()
{
	CIwTexture* tx = (CIwTexture*)IwGetResManager()->GetResNamed("logo", IW_GX_RESTYPE_TEXTURE);
	uint32 w1 = tx->GetWidth();
	uint32 h1 = tx->GetHeight();

	//static CIwSVec2 uvs[4] =
	//{
	//	CIwSVec2(0 << 12, 0 << 12),
	//	CIwSVec2(0 << 12, 1 << 12),
	//	CIwSVec2(1 << 12, 1 << 12),
	//	CIwSVec2(1 << 12, 0 << 12),
	//};

	static CIwSVec2 uvs[4] =
	{
		CIwSVec2((0 << 12) + 10, (0 << 12) + 10),
		CIwSVec2((0 << 12) + 10, (1 << 12) - 10),
		CIwSVec2((1 << 12) - 10, (1 << 12) - 10),
		CIwSVec2((1 << 12) - 10, (0 << 12) + 10),
	};
	
	int w = w1/2;
	int h = h1/2;

	int counter = 0;

	while (true)
	{
		IwGetHTTPQueue()->Update();
		IwGetNotificationHandler()->Update();
		IwGetMultiplayerHandler()->Update();

		IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);

		CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
		pMat->SetModulateMode(CIwMaterial::MODULATE_NONE);
		pMat->SetTexture(tx);
		IwGxSetMaterial(pMat);

		CIwMat rotMat;
		rotMat.SetIdentity();
		double perCentAngle = (counter / 80.0);
		iwangle degAng = (iwangle)(IW_GEOM_ONE * perCentAngle);

		rotMat.SetRotZ(degAng);
		rotMat.t.z = -0x200;

		IwGxSetViewMatrix(&rotMat);

		CIwSVec3* pWSCoords= IW_GX_ALLOC(CIwSVec3, 4);
		pWSCoords[0].x = -w; pWSCoords[0].y = -h;
		pWSCoords[1].x = -w; pWSCoords[1].y = h;
		pWSCoords[2].x = w; pWSCoords[2].y = h;
		pWSCoords[3].x = w; pWSCoords[3].y = -h;
		pWSCoords[0].z = pWSCoords[1].z = pWSCoords[2].z = pWSCoords[3].z = 0;

		if (!g_bInProgress)
		{
			MapTile* pNextDownload = GetNextDownload(NULL);
			if (pNextDownload)
			{
				IwGetNotificationHandler()->PushNotification((int)this, pNextDownload->szImageUrl, 10*1000);
				g_bInProgress = true;
				LoadMapTileImage(pNextDownload);
			}
			else
			{
				IwGetNotificationHandler()->ClearNotification((int)this);
				break;
			}
		}

		IwGxSetVertStreamWorldSpace(pWSCoords, 4);
		IwGxSetUVStream(uvs);
		IwGxDrawPrims(IW_GX_QUAD_LIST, NULL, 4);
		IwGetNotificationHandler()->Render();

		IwGxFlush();
		IwGxSwapBuffers();
		s3eDeviceYield(50);

		counter++;
	}
}

void MapBackground::EmptyTileList(std::list<MapTile*>* pAbortList, bool forceDelete)
{
	std::list<MapTile*>::iterator iter = pAbortList->begin();

	while (iter != pAbortList->end())
	{
		MapTile* pTile = *iter;
		AbortTile(pTile, forceDelete);
		iter++;
	}
	pAbortList->clear();
}

bool MapBackground::IsTileVisible(MapTile* pTile)
{
	float offset = 1 + (ceil(g_actualZoom) - g_actualZoom);
	int width = Iw2DGetSurfaceWidth() * offset;
	int height = Iw2DGetSurfaceHeight() * offset;

	return true;
	return (pTile->location.x > -_tileWidth) && (pTile->location.y > -_tileWidth) && (pTile->location.x < (width+_tileWidth)) && (pTile->location.y < (_tileWidth+height));
}

MapTile* MapBackground::GetNextDownload(MapTile* pCurrent)
{
	std::list<MapTile*>::iterator iter = gVectorImageUrls.begin();

	for (int i = 0; i < 2; ++i)
	{
		bool bFirstPass = (i == 0);
		while (iter != gVectorImageUrls.end())
		{
			MapTile* pTile = *iter;
			
			if ((!pTile->pTexture || pTile->pTexture == g_pLoadingTexture) && pTile != pCurrent)
			{
				bool isVisible = IsTileVisible(pTile);

				if (!bFirstPass || isVisible)
				{
					return pTile;
				}
			}

			iter++;
		}
	}
	return NULL;
}

void MapBackground::AbortTile(MapTile* pTile, bool forceDelete)
{
	if (pTile->pTexture && !forceDelete && false)
	{
		g_pTileCache.insert_slow(pTile, 0);

		for (int i = g_pTileCache.size()-1; i >= MAX_TILE_CACHE_COUNT; i--)
		{
			gDeleteImageUrls.push_back(g_pTileCache[i]);
			g_pTileCache.erase(i);
		}
	}
	else
	{
		if (pTile->pTexture)
		{
			if (g_pNotFoundTexture != pTile->pTexture && g_pLoadingTexture != pTile->pTexture)
			{
				delete (pTile->pTexture);
				pTile->pTexture = NULL;
			}
		}
		if (forceDelete)
		{
			delete pTile;
		}
		else
		{
			gDeleteImageUrls.push_back(pTile);
		}
	}
}

void MapBackground::CreateMapTileImage2(MapTile* pMapTile, char* szPath, bool isJpg)
{
	if (!IsTileVisible(pMapTile))
	{
		return;
	}
	CIwImage image;
	if (!isJpg)
	{
		image.LoadFromFile(szPath);
		if (image.GetWidth())
		{
			pMapTile->pTexture = new CIwTexture;
			pMapTile->pTexture->CopyFromImage(&image);
			pMapTile->pTexture->Upload();
		}
	}
	else
	{
		s3eFile* pFile = s3eFileOpen(szPath, "r");

		if (pFile)
		{
			uint32 gResultLen = s3eFileGetSize(pFile);
			void* gResult = (void*)s3eMalloc(gResultLen + 1);

			uint32 test = s3eFileRead(gResult, sizeof(char), gResultLen, pFile);
			gResultLen = test;
			s3eFileClose(pFile);

			JPEGImage(gResult, gResultLen, image);
			pMapTile->pTexture = new CIwTexture;
			pMapTile->pTexture->CopyFromImage(&image);
			pMapTile->pTexture->Upload();

			delete gResult;
		}
	}
}


void MapBackground::CreateMapTileImage(MapTile* pMapTile, void* gResult, uint32 gResultLen, bool isJpg)
{
	if (!IsTileVisible(pMapTile))
	{
		return;
	}
	CIwImage image;
	if (!isJpg)
	{
		//pMapTile->pTexture = g_pNotFoundTexture;
		s3eFile* tempfile = s3eFileOpenFromMemory((void*)gResult, gResultLen);
		image.ReadFile(tempfile);

		//image.LoadFromFile("tiles/r0302322130033033130.png");

		if (image.GetWidth())
		{
			pMapTile->pTexture = new CIwTexture;
			pMapTile->pTexture->CopyFromImage(&image);
			pMapTile->pTexture->Upload();
		}

		s3eFileClose(tempfile);
	}
	else
	{
		JPEGImage(gResult, gResultLen, image);
		pMapTile->pTexture = new CIwTexture;
		pMapTile->pTexture->CopyFromImage(&image);
		pMapTile->pTexture->Upload();
	}
}

void MapBackground::LoadMapTileImage(MapTile* pTile)
{
	char szImage[200], szPath[200];
	strcpy(szImage, pTile->szImageUrl);

	char* szName = strrchr(szImage, '/')+1;
	strchr(szName, '.')[0] = 0;
	strcat(szName, ".jpg");

	strcpy(szPath, "tiles/");
	strcat(szPath, szName);

	bool isJpg = true;
	if (!s3eFileCheckExists(szPath))
	{
		isJpg = false;
		strchr(szPath, '.')[0] = 0;
		strcat(szPath, ".png");

		char* szTiles = strchr(szPath, '/');
		szTiles[1] = 'r';
	}

	if (s3eFileCheckExists(szPath))
	{
		if ((!pTile->pTexture || pTile->pTexture == g_pLoadingTexture))
		{
			CreateMapTileImage2(pTile, szPath, isJpg);
		}
		g_bInProgress = false;
	}
	else
	{
		pTile->bInProgress = true;
		((ThisAndTile*)g_pThisAndTile)->pTile = pTile;

		IwGetHTTPQueue()->Get(pTile->szImageUrl, g_pThisAndTile, &MapBackground::GotImage, &MapBackground::GotImageError);
	}
}

void MapBackground::SaveMapTileImage(MapTile* pTile, void* gResult, uint32 gResultLen, bool isJpg)
{
	char szImage[200], szPath[200];
	strcpy(szImage, pTile->szImageUrl);

	char* szName = strrchr(szImage, '/')+1;
	strchr(szName, '.')[0] = 0;

	strcpy(szPath, "tiles/");
	strcat(szPath, szName);

	if (isJpg)
	{
		strcat(szPath, ".jpg");
	}
	else
	{
		strcat(szPath, ".png");
	}

	s3eFile* pFile = s3eFileOpen(szPath, "w");

	if (pFile)
	{
		uint32 x = s3eFileWrite(gResult, 1, gResultLen, pFile);
		s3eFileFlush(pFile);
		s3eFileClose(pFile);
	}
}

void MapBackground::GotImageError(void * pThisAndTileVoid)
{
	ThisAndTile* pThisAndTile = (ThisAndTile*)pThisAndTileVoid;
	MapBackground* pThis = pThisAndTile->pThis;
	MapTile* pMapTile = pThisAndTile->pTile;

	pMapTile->retryCount++;

	if (pMapTile->retryCount > 3 || true)
	{
		pMapTile->pTexture = g_pNotFoundTexture;
	}
	pThis->g_bInProgress = false;
	pMapTile->bInProgress = false;
}

void MapBackground::GotImage(void * pThisAndTileVoid, const char* szContentType, const char * Result, uint32 ResultLen)
{
	ThisAndTile* pThisAndTile = (ThisAndTile*)pThisAndTileVoid;
	MapBackground* pThis = pThisAndTile->pThis;
	MapTile* pMapTile = pThisAndTile->pTile;

	// Put the results pointer returned from CIwHTTP::Get into a s3eFile then create an CIwImage
	bool isJpg = false;
	bool isValid = false;

	char* szExt = strstr(pMapTile->szImageUrl, ".jpg");
	char* szTiles = strstr(pMapTile->szImageUrl, "tiles/");

	bool retryDownload = false;

	if (strstr(szContentType, "image/jpeg"))
	{
		isJpg = true;
		isValid = true;
	}
	else if (strstr(szContentType, "image/png"))
	{
		isJpg = false;
		isValid = true;
	}
	else
	{
		// we got a 404
		isValid = false;
	}

	if (strstr(pMapTile->szImageUrl, ".jpg"))
	{
		if (!isJpg && (pMapTile->retryCount == 0))
		{
			retryDownload = true;

			pMapTile->retryCount++;
			strcpy(pMapTile->szImageUrl, pMapTile->szImagePngUrl);
		}
	}

	pThis->g_bInProgress = false;
	pMapTile->bInProgress = false;

	if (retryDownload)
	{
		//
	}
	else
	{
		if (Result)
		{
			if (isValid)
			{
				pThis->SaveMapTileImage(pMapTile, (void*)Result, ResultLen, isJpg);
				pThis->LoadMapTileImage(pMapTile);
			}
			else
			{
				pMapTile->pTexture = g_pNotFoundTexture;
			}
			//pThis->CreateMapTileImage(pMapTile, (void*)Result, ResultLen, isJpg);
		}
		else
		{
			pMapTile->pTexture = g_pNotFoundTexture;
		}
	}
}

int32 MapBackground::GotData(void* pDownloaderVoid, void* pThisAndTileVoid)
{
	CIwHTTP* pDownloader = (CIwHTTP*)pDownloaderVoid;
	ThisAndTile* pThisAndTile = (ThisAndTile*)pThisAndTileVoid;
	MapBackground* pThis = pThisAndTile->pThis;
	MapTile* pMapTile = pThisAndTile->pTile;

    // This is the callback indicating that a ReadContent call has
	// completed.  Either we've finished, or a bigger buffer is
	// needed.  If the correct ammount of data was supplied initially,
	// then this will only be called once. However, it may well be
	// called several times when using chunked encoding.

    // Firstly see if there's an error condition.
    if (pDownloader->GetStatus() == S3E_RESULT_ERROR)
    {
        // Something has gone wrong
        //status = kOK;
		s3eDebugTraceLine("Error downloading tile data - error");
    }
	else if (pDownloader->ContentReceived() != pDownloader->ContentLength())
	{
		s3eDebugTraceLine("Error downloading tile data - length");

		// We have some data but not all of it. We need more space.
		uint32 oldLen = pThis->gResultLen;

        // If iwhttp has a guess how big the next bit of data is (this
        // basically means chunked encoding is being used), allocate
        // that much space. Otherwise guess.
		if (pThis->gResultLen < pDownloader->ContentExpected())
		{
			pThis->gResultLen = pDownloader->ContentExpected();
		}
		else
		{
			pThis->gResultLen += 1024;
		}

        // Allocate some more space and fetch the data.
		pThis->gResult = (char*)s3eRealloc(pThis->gResult, pThis->gResultLen);
		pDownloader->ReadContent(&pThis->gResult[oldLen], pThis->gResultLen - oldLen, &MapBackground::GotData);
	}
	else
	{

		// Put the results pointer returned from CIwHTTP::Get into a s3eFile then create an CIwImage
		_STL::string imageType;
		bool isJpg = false;
		bool isValid = false;
		pDownloader->GetHeader("content-type", imageType);

		if (strstr(imageType.c_str(), "image/jpeg"))
		{
			isJpg = true;
			isValid = true;
		}
		else if (strstr(imageType.c_str(), "image/png"))
		{
			isJpg = false;
			isValid = true;
		}
		else
		{
			// we got a 404
			isValid = false;
		}

		if (pThis->gResult)
		{
			pThis->CreateMapTileImage(pMapTile, pThis->gResult, pThis->gResultLen, isJpg);

			if (isValid)
			{
				pThis->SaveMapTileImage(pMapTile, pThis->gResult, pThis->gResultLen, isJpg);
			}
		}
	}

	pThis->g_bInProgress = false;
	pMapTile->bInProgress = false;

	return 0;
}

/*
//-----------------------------------------------------------------------------
// Called when the response headers have been received
//-----------------------------------------------------------------------------
int32 MapBackground::GotHeaders(void* pDownloaderVoid, void* pThisAndTileVoid)
{
	CIwHTTP* pDownloader = (CIwHTTP*)pDownloaderVoid;
	ThisAndTile* pThisAndTile = (ThisAndTile*)pThisAndTileVoid;
	MapBackground* pThis = pThisAndTile->pThis;
	MapTile* pMapTile = pThisAndTile->pTile;

    if (pDownloader->GetStatus() == S3E_RESULT_ERROR)
    {
        // Something has gone wrong
        //status = kError;
		s3eDebugTraceLine("Error downloading tile header");
		pDownloader->Cancel();
		pMapTile->pTexture = g_pNotFoundTexture;
		pMapTile->bInProgress = false;

		pThis->g_bInProgress = false;
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
		pThis->gResultLen = pDownloader->ContentExpected();

		if (!pThis->gResultLen)
		{
			pThis->gResultLen = 4096;
		}

		if (pThis->gResult)
		{
			delete pThis->gResult;
		}
		pThis->gResult = (char*)s3eMalloc(pThis->gResultLen + 1);
		pThis->gResult[pDownloader->ContentExpected()] = 0;
		pDownloader->ReadContent(pThis->gResult, pDownloader->ContentLength(), GotData,  pThisAndTile);
    }
    return 0;
}
*/

void MapBackground::ProcessNewDownloads(std::list<MapTile*>* pNewTiles)
{
	std::list<MapTile*>::iterator iter1 = gVectorImageUrls.begin();
	std::list<MapTile*> deleteList;

	while (iter1 != gVectorImageUrls.end())
	{
		std::list<MapTile*>::iterator iter2 = pNewTiles->begin();
		bool bFound = false;

		while (iter2 != pNewTiles->end())
		{
			if (0 == stricmp((*iter1)->szImageUrl, (*iter2)->szImageUrl))
			{
				MapTile* pTile1 = *iter1;
				MapTile* pTile2 = *iter2;

				pNewTiles->insert(iter2, pTile1);
				pNewTiles->remove(pTile2);

				pTile1->location.x = pTile2->location.x;
				pTile1->location.y = pTile2->location.y;

				pTile1->row = pTile2->row;
				pTile1->col = pTile2->col;
				pTile1->retryCount = pTile2->retryCount;

				delete pTile2;
				bFound = true;
				break;
			}
			iter2++;
		}

		if (!bFound)
		{
			deleteList.push_back(*iter1);
		}

		iter1++;
	}
	gVectorImageUrls.clear();

	EmptyTileList(&deleteList, false);

	iter1 = pNewTiles->begin();
	MapTile* pFirst = NULL;
	while (iter1 != pNewTiles->end())
	{
		MapTile* pTile = *iter1;

		if (!pTile->pTexture)
		{
			for (int i = 0; i < g_pTileCache.size(); ++i)
			{
				if (0 == stricmp(g_pTileCache[i]->szImageUrl, pTile->szImageUrl))
				{
					pTile->pTexture = g_pTileCache[i]->pTexture;
					delete g_pTileCache[i];
					g_pTileCache.erase(i);
					break;
				}
			}
			if (!pTile->pTexture)
			{
				pTile->pTexture = g_pLoadingTexture;
			}
		}
		gVectorImageUrls.push_back(pTile);

		iter1++;
	}
}

CIwSVec2 MapBackground::GetPosition()
{
	CIwSVec2 position;

	GetScaler()->LocationToPosition(gLocation, &position);

	return position;
}

void MapBackground::SetLocation(double longitude, double latitude)
{
	gLocation.m_Longitude = longitude;
	gLocation.m_Latitude = latitude;

	g_bLocationChanged = true;

}

CoordinateScaler* MapBackground::GetScaler()
{
	return (g_bScaledMode) ? gScaledModeScaler : gScaler;
}

bool MapBackground::IsInitialized()
{
	return !g_bInitialLoad;
}

bool MapBackground::IsScaled()
{
	return g_bScaledMode;
}

bool MapBackground::IsAnimating()
{
	return g_bIsAnimating;
}

double MapBackground::GetLatitudePerPixel()
{
	return g_latPerPixel;
}

double MapBackground::GetLongitudePerPixel()
{
	return g_lonPerPixel;
}

void MapBackground::SetScaledCorners(s3eLocation& topLeft, s3eLocation& topRight, s3eLocation& bottomLeft, s3eLocation& bottomRight, float zoom)
{
    double maxIncreasing = 0;
    s3eLocation* maxPos1 = NULL;
	s3eLocation* maxPos2 = NULL;

	s3eLocation locations[4];
	locations[0] = topLeft;
	locations[1] = topRight;
	locations[2] = bottomLeft;
	locations[3] = bottomRight;

    for (int i = 0; i < 4; ++i)
    {
		s3eLocation* startPos = &locations[i];
        for (int j = 0; j < 4; ++j)
        {
			s3eLocation* endPos = &locations[j];
			double pointDistance = LiveMaps::CalculateDistance(*startPos, *endPos);

            if (pointDistance > maxIncreasing)
            {
                maxIncreasing = pointDistance;
                maxPos1 = startPos;
                maxPos2 = endPos;
            }
        }
    }

    double slopeAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(maxPos1, maxPos2);
	s3eLocation midPoint;

    LiveMaps::CalculateLatLongInDirection(maxPos1, maxIncreasing, slopeAngle, &midPoint);
    //s3eLocation midPoint = 
    double bestIncAngle = PI / 2.0, bestDecAngle = PI / 2.0;
    s3eLocation* bestThirdPos = NULL;
	s3eLocation* bestFourthPos = NULL;

    int index = 1;
    for (int i = 0; i < 4; ++i)
    {
		s3eLocation* anglePos = &locations[i];
        double angle = LiveMaps::CalculateTriangleAngles(*maxPos1, *anglePos, *maxPos2);
        index++;

        double testSlopeAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(maxPos1, anglePos);

        if (testSlopeAngle < 0)
        {
            testSlopeAngle += PI * 2;
        }

        bool increasing = (testSlopeAngle < slopeAngle);
        double angleDiff = ABS(ABS(angle) - PI / 2.0);

        if (increasing)
        {
            if (angleDiff < bestIncAngle)
            {
                bestIncAngle = angleDiff;
                bestThirdPos = anglePos;
            }
        }
        else
        {
            if (angleDiff < bestDecAngle)
            {
                bestDecAngle = angleDiff;
                bestFourthPos = anglePos;
            }
        }
    }
    if (NULL == maxPos1 || NULL == maxPos2 || NULL == bestThirdPos || NULL == bestFourthPos)
    {
        //
    }
	if (NULL == bestThirdPos)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (&locations[i] != maxPos1 && &locations[i] != maxPos2 && &locations[i] != bestFourthPos)
			{
				bestThirdPos = &locations[i];
				break;
			}
		}
	}

	if (NULL == bestFourthPos)
	{
		for (int i = 0; i < 4; ++i)
		{
			if (&locations[i] != maxPos1 && &locations[i] != maxPos2 && &locations[i] != bestThirdPos)
			{
				bestFourthPos = &locations[i];
				break;
			}
		}
	}

	GPSRectangle rect;
	int width, height;

    OrientCoordinates(maxPos1, maxPos2, bestThirdPos, bestFourthPos, &width, &height, rect);
	memcpy(&g_scaledCorners, &rect, sizeof(GPSRectangle));
	
	g_tempZoom = g_actualZoom = g_newZoom = g_curZoom = zoom;

	//int zoom = LiveMaps::EstimateZoom(&g_scaledCorners, Iw2DGetSurfaceWidth(), Iw2DGetSurfaceHeight(), LiveMaps::MaxZoom);
	//g_curZoom = zoom;//LiveMaps::MaxZoom-1;
	Update(true);

	//RenderBackgroundOnSurface(g_pScaledSurface);
	gScaledModeScaler->SetCorners(&g_scaledCorners, g_curZoom, true);

	gLocation.m_Latitude = gScaledModeScaler->GetCenterLatitude();
	gLocation.m_Longitude = gScaledModeScaler->GetCenterLongitude();
	g_bLocationChanged = true;
	
	Update(true);

	// Update here to get the actual corners
	g_bScaledMode = true;
	CalculatePosition(true);
	CalculateTiles();
	gScaledModeScaler->SetBaseScaler(gScaler);
	//gScaler->SetCorners(&g_corners, g_curZoom, false);
	gScaler->SetCorners(&g_corners, g_actualZoom, false);

	g_iAnimationIndex = 0;
	g_bIsAnimating = true;
}


void MapBackground::OrientCoordinates(s3eLocation* a, s3eLocation* b, s3eLocation* c, s3eLocation* d, int* width, int* height, GPSRectangle& rect)
{
    s3eLocation* topLeft, *topRight, *bottomLeft, *bottomRight;

    if (a->m_Latitude > b->m_Latitude)
    {
        topLeft = a;
        bottomLeft = b;
    }
    else
    {
        topLeft = b;
        bottomLeft = a;
    }
    if (c->m_Latitude > d->m_Latitude)
    {
        topRight = c;
        bottomRight = d;
    }
    else
    {
        topRight = d;
        bottomRight = c;
    }

	double maxIncreasing = LiveMaps::CalculateDistance(bottomLeft, topRight);
    double maxDecreasing = LiveMaps::CalculateDistance(topLeft, bottomRight);

    if (topRight->m_Longitude < topLeft->m_Longitude)
    {
        a = topLeft;
        topLeft = topRight;
        topRight = a;
    }
    if (bottomRight->m_Longitude < bottomLeft->m_Longitude)
    {
        a = bottomLeft;
        bottomLeft = bottomRight;
        bottomRight = a;
    }
    if (bottomLeft->m_Latitude > topLeft->m_Latitude)
    {
        a = bottomLeft;
        bottomLeft = topLeft;
        topLeft = a;
    }
    if (bottomRight->m_Latitude > topRight->m_Latitude)
    {
        a = bottomRight;
        bottomRight = topRight;
        topRight = a;
    }

    // Now we have our 4 "corners"
    // We need to turn those into a valid rectangle

    double distanceTop, distanceBottom, distanceLeft, distanceRight;
    distanceTop = LiveMaps::CalculateDistance(topLeft, topRight);
    distanceBottom = LiveMaps::CalculateDistance(bottomLeft, bottomRight);
    distanceLeft = LiveMaps::CalculateDistance(topLeft, bottomLeft);
    distanceRight = LiveMaps::CalculateDistance(topRight, bottomRight);

    double topLeftAngle, topRightAngle, bottomLeftAngle, bottomRightAngle;
    double tempAngleA, tempAngleB;

    //System.Diagnostics.Debug.WriteLine("Calculating topLeftAngle");
    LiveMaps::CalculateTriangleAngles(distanceLeft, distanceTop, maxIncreasing, &tempAngleA, &tempAngleB, &topLeftAngle);
    //System.Diagnostics.Debug.WriteLine("Calculating topRightAngle");
    LiveMaps::CalculateTriangleAngles(distanceTop, distanceRight, maxDecreasing, &tempAngleA, &tempAngleB, &topRightAngle);
    //System.Diagnostics.Debug.WriteLine(String.Format("Calculating bottomRightAngle {0}, {1}, {2}", distanceRight, distanceBottom, maxIncreasing));
    LiveMaps::CalculateTriangleAngles(distanceRight, distanceBottom, maxIncreasing, &tempAngleA, &tempAngleB, &bottomRightAngle);
    //System.Diagnostics.Debug.WriteLine("Calculating bottomLeftAngle");
    LiveMaps::CalculateTriangleAngles(distanceBottom, distanceLeft, maxDecreasing, &tempAngleA, &tempAngleB, &bottomLeftAngle);

    topRightAngle = bottomRightAngle = 0;

    // We know our 4 angles now. Pick the one closest to 90 degrees (pi / 2)
    double ninetyDegrees = PI / 2;

    double
        tlDiff = ABS(ninetyDegrees - topLeftAngle),
        trDiff = ABS(ninetyDegrees - topRightAngle),
        blDiff = ABS(ninetyDegrees - bottomLeftAngle),
        brDiff = ABS(ninetyDegrees - bottomRightAngle);

    s3eLocation posFromTop, posFromBottom;

    //System.Diagnostics.Debug.WriteLine(String.Format("angle diffs: {0} - {1} - {2} - {3}", tlDiff, trDiff, blDiff, brDiff));

    //_topLeft = topLeft;
    //_topRight = topRight;
    //_bottomLeft = bottomLeft;
    //_bottomRight = bottomRight;
    //return;

	GPSRectangle tempRect;
	s3eLocation *_topLeft, *_topRight, *_bottomLeft, *_bottomRight;
	_topLeft = &tempRect.topLeft;
	_topRight = &tempRect.topRight;
	_bottomLeft = &tempRect.bottomLeft;
	_bottomRight = &tempRect.bottomRight;

    if (tlDiff <= trDiff && tlDiff <= blDiff && tlDiff <= brDiff)
    {
        // top left is our best angle
        _topLeft = topLeft;
        double angleIncrement = (ninetyDegrees - topLeftAngle) / 2;

        double topAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(topLeft, topRight);
        double leftAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(topLeft, bottomLeft);

        LiveMaps::ScaleAnglesTo90(&topAngle, &leftAngle, angleIncrement);

        LiveMaps::CalculateLatLongInDirection(topLeft, distanceTop, topAngle, _topRight);
        LiveMaps::CalculateLatLongInDirection(topLeft, distanceLeft, leftAngle, _bottomLeft);

        // We should be able to calculat bottomRight 2 ways
        LiveMaps::CalculateLatLongInDirection(_topRight, distanceLeft, leftAngle, &posFromTop);
        LiveMaps::CalculateLatLongInDirection(_bottomLeft, distanceTop, topAngle, &posFromBottom);

        _bottomRight->m_Latitude = (posFromBottom.m_Latitude + posFromTop.m_Latitude) / 2;
		_bottomRight->m_Longitude = (posFromBottom.m_Longitude + posFromTop.m_Longitude) / 2;
    }
    else if (trDiff <= tlDiff && trDiff <= blDiff && trDiff <= brDiff)
    {
        // top right is our best angle
        _topRight = topRight;
        double angleIncrement = (ninetyDegrees - topRightAngle) / 2;

        double topAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(topRight, topLeft);
        double rightAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(topRight, bottomRight);

        topAngle = topAngle + angleIncrement;
        rightAngle = rightAngle - angleIncrement;

        angleIncrement = (PI / 2 - (rightAngle - topAngle)) / 2;

        topAngle = topAngle + angleIncrement;
        rightAngle = rightAngle - angleIncrement;

        LiveMaps::CalculateLatLongInDirection(topRight, distanceTop, topAngle, _topLeft);
        LiveMaps::CalculateLatLongInDirection(topRight, distanceRight, rightAngle, bottomRight);

        // We should be able to calculat bottomRight 2 ways
        LiveMaps::CalculateLatLongInDirection(_bottomRight, distanceLeft, topAngle, &posFromTop);
        LiveMaps::CalculateLatLongInDirection(_topLeft, distanceTop, rightAngle, &posFromBottom);

        _bottomLeft->m_Latitude = (posFromBottom.m_Latitude + posFromTop.m_Latitude) / 2;
		_bottomLeft->m_Longitude = (posFromBottom.m_Longitude + posFromTop.m_Longitude) / 2;
    }
    else if (blDiff <= tlDiff && blDiff <= trDiff && blDiff <= brDiff)
    {
        // bottom left is our best angle
        _bottomLeft = bottomLeft;
        double angleIncrement = (ninetyDegrees - bottomLeftAngle) / 2;

        double leftAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(bottomLeft, topLeft) - angleIncrement;
        double bottomAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(bottomLeft, bottomRight) + angleIncrement;

        angleIncrement = (leftAngle - bottomAngle - PI / 2) / 2;

        leftAngle = leftAngle - angleIncrement;
        bottomAngle = bottomAngle + angleIncrement;

        LiveMaps::CalculateLatLongInDirection(bottomLeft, distanceLeft, leftAngle, _topLeft);
        LiveMaps::CalculateLatLongInDirection(bottomLeft, distanceBottom, bottomAngle, _bottomRight);

        // We should be able to calculat bottomRight 2 ways
        LiveMaps::CalculateLatLongInDirection(_bottomRight, distanceLeft, leftAngle, &posFromTop);
        LiveMaps::CalculateLatLongInDirection(_topLeft, distanceTop, bottomAngle, &posFromBottom);

        _topRight->m_Latitude = (posFromBottom.m_Latitude + posFromTop.m_Latitude) / 2;
		_topRight->m_Longitude = (posFromBottom.m_Longitude + posFromTop.m_Longitude) / 2;
        //System.Diagnostics.Debug.WriteLine(String.Format("c - {0} should equal {1} = {2}", posFromBottom, posFromTop, b));
    }
    else
    {
        // bottom right is our best angle
        _bottomRight = bottomRight;
        double angleIncrement = (ninetyDegrees - bottomRightAngle) / 2;

        double rightAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(bottomRight, topRight) + angleIncrement;
        double bottomAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(bottomRight, bottomLeft) - angleIncrement;

        angleIncrement = (2 * PI + bottomAngle - rightAngle - PI / 2) / 2;

        rightAngle = rightAngle + angleIncrement;
        bottomAngle = bottomAngle - angleIncrement;

        LiveMaps::CalculateLatLongInDirection(bottomRight, distanceBottom, rightAngle, _topRight);
        LiveMaps::CalculateLatLongInDirection(bottomRight, distanceRight, bottomAngle, _bottomLeft);

        // We should be able to calculat bottomRight 2 ways
        LiveMaps::CalculateLatLongInDirection(_topRight, distanceRight, rightAngle, &posFromTop);
        LiveMaps::CalculateLatLongInDirection(_bottomLeft, distanceBottom, bottomAngle, &posFromBottom);

        _topLeft->m_Latitude = (posFromBottom.m_Latitude + posFromTop.m_Latitude) / 2;
		_topLeft->m_Longitude = (posFromBottom.m_Longitude + posFromTop.m_Longitude) / 2;
        //System.Diagnostics.Debug.WriteLine(String.Format("d - {0} should equal {1} = {2}", posFromBottom, posFromTop, a));
    }
    //System.Diagnostics.Debug.WriteLine(String.Format("diff = {0}", CalculateDistance(posFromTop, posFromBottom)));

    double a1 = LiveMaps::CalculateSlopeAngleBetweenPoints(_topLeft, _topRight);
    double a2 = LiveMaps::CalculateSlopeAngleBetweenPoints(_topLeft, _bottomLeft);

    //System.Diagnostics.Debug.WriteLine(String.Format("slope1 = {0}, slope2 = {1} -- {2}", a1, a2, a1 - a2));

    //_drawnCoordinates.Sort(new Comparison<s3eLocation>(CompareCoordinateLatitude));
    // top 2 are the farthest left

    if (topRight->m_Longitude < topLeft->m_Longitude)
    {
        _topRight = topLeft;
        _topLeft = topRight;
    }
    if (bottomRight->m_Longitude < bottomLeft->m_Longitude)
    {
        _bottomRight = bottomLeft;
        _bottomLeft = bottomRight;
    }

	memcpy(&rect.topLeft, _topLeft, sizeof(s3eLocation));
	memcpy(&rect.topRight, _topRight, sizeof(s3eLocation));
	memcpy(&rect.bottomLeft, _bottomLeft, sizeof(s3eLocation));
	memcpy(&rect.bottomRight, _bottomRight, sizeof(s3eLocation));

    *width = (int)MAX(LiveMaps::CalculateDistance(_topLeft, _topRight), LiveMaps::CalculateDistance(_bottomLeft, _bottomRight));
    *height = (int)MAX(LiveMaps::CalculateDistance(_topLeft, _bottomLeft), LiveMaps::CalculateDistance(_topRight, _bottomRight));
}

void MapBackground::ClearScaledCorners()
{
	g_dAlpha = 0xFF;
	g_bScaledMode = false;
	g_viewMatrix.SetIdentity();
	g_viewMatrix.t.z = -0x200;

	IwGxSetViewMatrix(&g_viewMatrix);

	delete gScaledModeScaler;
	gScaledModeScaler = new CoordinateScaler(Iw2DGetSurfaceWidth(), Iw2DGetSurfaceHeight(), NULL, 0, true);

	Update(true);
}

void MapBackground::GetLocation(s3eLocation& pLocation)
{
	pLocation.m_Latitude = gLocation.m_Latitude;
	pLocation.m_Longitude = gLocation.m_Longitude;
}

bool MapBackground::UpdatePositionFromMouseAndKeyboard(CIwSVec2& clickPos, bool* pbClicked)
{
	bool forceUpdate = false;
	bool downLoad = false;

	if (this->IsInitialized())
	{
		bool updateLocation = false;
		CIwSVec2 pos;
		s3eLocation newLocation;
		this->GetScaler()->LocationToPosition(gLocation, &pos);

		if (s3eKeyboardGetState(s3eKeyUp) & S3E_KEY_STATE_DOWN)
		{
			pos.y -= 10;
			this->GetScaler()->PositionToLocation(pos, &newLocation);
			if (this->IsScaled())
			{
				gLocation = newLocation;
			}
			else
			{
				gLocation.m_Latitude = newLocation.m_Latitude;
			}
			updateLocation = true;
		}
		if (s3eKeyboardGetState(s3eKeyDown) & S3E_KEY_STATE_DOWN)
		{
			pos.y += 10;
			this->GetScaler()->PositionToLocation(pos, &newLocation);
			if (this->IsScaled())
			{
				gLocation = newLocation;
			}
			else
			{
				gLocation.m_Latitude = newLocation.m_Latitude;
			}
			updateLocation = true;
		}
		if (s3eKeyboardGetState(s3eKeyLeft) & S3E_KEY_STATE_DOWN)
		{
			pos.x -= 10;
			this->GetScaler()->PositionToLocation(pos, &newLocation);
			if (this->IsScaled())
			{
				gLocation = newLocation;
			}
			else
			{
				gLocation.m_Longitude = newLocation.m_Longitude;
			}
			g_bShowCursor = false;
			updateLocation = true;
		}
		if (s3eKeyboardGetState(s3eKeyRight) & S3E_KEY_STATE_DOWN)
		{
			pos.x += 10;
			this->GetScaler()->PositionToLocation(pos, &newLocation);
			if (this->IsScaled())
			{
				gLocation = newLocation;
			}
			else
			{
				gLocation.m_Longitude = newLocation.m_Longitude;
			}
			g_bShowCursor = false;
			updateLocation = true;
		}
		
		if (g_iActiveTouches > 1)
		{
			if (!g_bMultiTouch)
			{
				g_multitouchZoom = g_actualZoom;
				g_bMultiTouch = true;
			}
			CTouch* pTouch1 = NULL;
			CTouch *pTouch2 = NULL;

			g_iActiveTouches = 0;
			for (uint i = 0; i < MAX_TOUCHES; ++i)
			{
				if (g_Touches[i].active)
				{
					g_iActiveTouches++;
					if (!pTouch1)
					{
						pTouch1 = &g_Touches[i];
					}
					else if (!pTouch2)
					{
						pTouch2 = &g_Touches[i];
					}
				}
			}

			if (pTouch1 && pTouch2)
			{
				int startDistance = sqrt(pow(pTouch1->startX - pTouch2->startX, 2) + pow(pTouch1->startY - pTouch2->startY, 2));
				int currDistance = sqrt(pow(pTouch1->x - pTouch2->x, 2) + pow(pTouch1->y - pTouch2->y, 2));
				
				int distance = MIN(50, (startDistance - currDistance));

				g_tempZoom = g_newZoom = g_actualZoom = MIN(19, MAX(14, g_multitouchZoom - ((float)distance / 25)));
				g_bLocationChanged = true;
			}
		}
		//else if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN)
		//{
		//	CIwSVec2 currentPos;
		//	g_bShowCursor = false;
		//	updateLocation = true;

		//	currentPos.x = (int16)s3ePointerGetX();
		//	currentPos.y = (int16)s3ePointerGetY();

		//	if (g_bMouseDown)
		//	{
		//		if (ABS(currentPos.x - g_downMousePos.x) > 5 || ABS(currentPos.y - g_downMousePos.y) > 5)
		//		{
		//			gLocation.m_Longitude += -(currentPos.x - g_lastMousePos.x) * this->GetLongitudePerPixel();
		//			gLocation.m_Latitude += -(currentPos.y - g_lastMousePos.y) * this->GetLatitudePerPixel();
		//			forceUpdate = true;
		//		}
		//	}
		//	else
		//	{
		//		g_downMousePos.x = currentPos.x;
		//		g_downMousePos.y = currentPos.y;
		//	}

		//	g_lastMousePos.x = currentPos.x;
		//	g_lastMousePos.y = currentPos.y;

		//	g_bMouseDown = true;
		//}
		else if (s3ePointerGetState(S3E_POINTER_BUTTON_RIGHTMOUSE) & S3E_POINTER_STATE_RELEASED)
		{
			forceUpdate = true;
			g_newZoom = g_actualZoom - .25;
		}
		else
		{
			g_bMultiTouch = false;
			if (g_bMouseDown)
			{
				if (ABS(g_downMousePos.x - g_lastMousePos.x) < 5 && ABS(g_downMousePos.y - g_lastMousePos.y) < 5)
				{
					*pbClicked = true;
					clickPos.x = g_lastMousePos.x;
					clickPos.y = g_lastMousePos.y;
				}
			}
			g_bMouseDown = false;
		}
		if (updateLocation)
		{
			this->SetLocation(gLocation.m_Longitude, gLocation.m_Latitude);
		}
	}
	if (g_bInitialLoad)
	{
		g_bInitialLoad = false;
		downLoad = true;
	}

	double	dLat = gPrevLocation.m_Latitude - gLocation.m_Latitude;
	double	dLng = gPrevLocation.m_Longitude - gLocation.m_Longitude;

	if (dLat < 0)
	{
		dLat = -dLat;
	}
	if (dLng < 0)
	{
		dLng = -dLng;
	}

	if ((dLat > 0.00050 || dLng  > 0.00050))
	{
		downLoad = true;
	}

	if (downLoad)
	{
		gPrevLocation = gLocation;
	}
	this->Update(downLoad || forceUpdate);

	return forceUpdate;
}


bool MapBackground::CalculatePosition(bool forceDownload)
{
	bool downLoad = forceDownload;

	if (g_bScaledMode)
	{
		gLocation.m_Longitude = gScaledModeScaler->GetCenterLongitude();
		gLocation.m_Latitude = gScaledModeScaler->GetCenterLatitude();
	}

	if (gError == S3E_RESULT_SUCCESS)
	{
		if (g_bInitialLoad)
		{
			g_bInitialLoad = false;
			downLoad = true;
		}

		double	dLat = gPrevLocation.m_Latitude - gLocation.m_Latitude;
		double	dLng = gPrevLocation.m_Longitude - gLocation.m_Longitude;

		if (dLat < 0)
		{
			dLat = -dLat;
		}
		if (dLng < 0)
		{
			dLng = -dLng;
		}

		CIwSVec2 curPos, prevPos;

		GetScaler()->LocationToPosition(gLocation, &curPos);
		GetScaler()->LocationToPosition(gPrevLocation, &prevPos);

		if (ABS(prevPos.y - curPos.y) > Iw2DGetSurfaceHeight() / 4)
		{
			downLoad = true;
		}
		else if (ABS(prevPos.x - curPos.x) > Iw2DGetSurfaceWidth() / 4)
		{
			downLoad = true;
		}

		//if ((dLat > 0.00050 || dLng  > 0.00050))
		//{
		//	downLoad = true;		
		//}
	}
	else
	{
		gLocation.m_Latitude = 51;
		gLocation.m_Longitude = -0.1;
		
		double	dLat = gPrevLocation.m_Latitude - gLocation.m_Latitude;
		double	dLng = gPrevLocation.m_Longitude - gLocation.m_Longitude;

		if (dLat < 0)
		{
			dLat = -dLat;
		}
		if (dLng < 0)
		{
			dLng = -dLng;
		}

		if (g_bInitialLoad)
		{
			downLoad = true;
			g_bInitialLoad = false;
		}

		if ((dLat > 0.00001 || dLng  > 0.00001))
		{
			downLoad = true;		
		}
	}

	bool updateView = false;
	if (g_newZoom != g_actualZoom)
	{
		g_bLocationChanged = true;
		g_tempZoom = g_actualZoom;
		g_actualZoom = g_newZoom;
		downLoad = true;
	}

	if (g_tempZoom < g_actualZoom)
	{
		g_tempZoom += .05;

		if (g_tempZoom > g_actualZoom)
		{
			g_tempZoom = g_actualZoom;
		}
		g_bLocationChanged = true;
		downLoad = true;
	}
	if (g_tempZoom > g_actualZoom)
	{
		g_tempZoom -= .05;

		if (g_tempZoom < g_actualZoom)
		{
			g_tempZoom = g_actualZoom;
		}
		g_bLocationChanged = true;
		downLoad = true;
	}

	if (downLoad)
	{
		gPrevLocation = gLocation;
	}

	return downLoad;
}

bool MapBackground::CalculateTiles()
{
	if (g_bLocationChanged)
	{
		g_bLocationChanged = false;
		GPSRectangle x;
		std::list<MapTile*> vectorImages;

		int width = Iw2DGetSurfaceWidth();
		int height = Iw2DGetSurfaceHeight();

		float offset = 1 + (ceil(g_tempZoom) - g_tempZoom);
		LiveMaps::GetLocationImages(&vectorImages, &g_rows, &g_cols, &gLocation, (int)(width * offset), (int)(height * offset), &g_corners, &x, ceil(g_tempZoom), &g_latPerPixel, &g_lonPerPixel);

		int count = vectorImages.size();

		if (offset > 1)
		{
			int xOffset = width * (offset - 1) / 2;
			int yOffset = height * (offset - 1) / 2;

			std::list<MapTile*>::iterator iter1 = vectorImages.begin();

			while (iter1 != vectorImages.end())
			{
				MapTile* pTile = *iter1;

				pTile->location.x -= xOffset;
				pTile->location.y -= yOffset;

				iter1++;
			}
		}

		ProcessNewDownloads(&vectorImages);
		gScaler->SetCorners(&g_corners, g_actualZoom, false);

		g_Width = s3eSurfaceGetInt(S3E_SURFACE_WIDTH);
		g_Height = s3eSurfaceGetInt(S3E_SURFACE_HEIGHT);
	}

	return true;
}

void MapBackground::Clone(CIwUIElement* pTarget) const
{
	s3eDeviceYield(0);
}
CIwUIRect MapBackground::GetElementBounds() const
{
	return CIwUIRect();
}
void MapBackground::OnPropertyChanged(const char* pName)
{
	s3eDeviceYield(0);
}
void MapBackground::OnSizeChanged()
{
	s3eDeviceYield(0);
}
CIwVec2 MapBackground::MeasureElement(const CIwVec2& availableSize) const
{
	return CIwVec2(availableSize.x, availableSize.y);
}
void MapBackground::Animate(const CIwUIAnimData& animData)
{
	s3eDeviceYield(0);
}

void MapBackground::Render(CIwUIGraphics& parentGraphics)
{
	RenderElement(parentGraphics);
}

void MapBackground::RenderElement(CIwUIGraphics& parentGraphics)
{
	// Render tiles for the background
	std::list<MapTile*>::iterator iter = gVectorImageUrls.begin();

	int i = 0;
	while (iter != gVectorImageUrls.end())
	{
		MapTile* pTile = *iter;

		if (pTile->pTexture)
		{
			//Calculate the top left of the map image
			CIwSVec2 topLeft, bottomRight;
			topLeft.x = pTile->location.x;
			topLeft.y = pTile->location.y;
			CIwUIRect rect(CIwVec2(topLeft.x, topLeft.y), CIwVec2(pTile->pTexture->GetWidth(), pTile->pTexture->GetHeight()));

			CIwUIColour c(0xff,0xff,0xff,0xff);
			CIwColour* wtf = IW_GX_ALLOC(CIwColour, 4);

			CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
			pMat->SetModulateMode(CIwMaterial::MODULATE_NONE);
			pMat->SetTexture(pTile->pTexture);

			parentGraphics.DrawImage(pTile->pTexture, pMat, rect, CIwSVec2(0,0), CIwSVec2(4096, 4096), c, false);
		}
		iter++;
	}
}

bool MapBackground::Update(bool forceUpdate = false)
{
	if (gDeleteImageUrls.size() > 0)
	{
		std::list<MapTile*>::iterator iter = gDeleteImageUrls.begin();
		std::list<MapTile*> delList;

		while (iter != gDeleteImageUrls.end())
		{
			MapTile* delTile = *iter;
			
			if (!delTile->bInProgress || !g_bInProgress)
			{
				delList.push_back(delTile);
			}
			iter++;
		}
		iter = delList.begin();
		while (iter != delList.end())
		{
			MapTile* delTile = *iter;

			gDeleteImageUrls.remove(delTile);

			delete delTile;
			iter++;
		}
	}

	bool setViewMatrix = false;
	double scaleX, scaleY, perCentAngle;
	if (true)
	{
		if (CalculatePosition(forceUpdate) || g_bLocationChanged)
		{
			CalculateTiles();
		}
		if (g_bScaledMode)
		{
			double rotAngle = gScaledModeScaler->GetRotationAngle();
			perCentAngle = LiveMaps::RadToDeg(rotAngle) / 360;

			scaleX = gScaledModeScaler->GetScaleX();
			scaleY = gScaledModeScaler->GetScaleY();
			setViewMatrix = true;
			g_viewMatrix = gScaledModeScaler->GetDrawRotationMatrix(1.0);
		}
		else
		{
			float offset = 1 - (ceil(g_tempZoom) - g_tempZoom) / 2;

			CIwMat viewMatrix;
			viewMatrix.SetIdentity();
			viewMatrix.t.x = 0;
			viewMatrix.t.y = 0;
			viewMatrix.t.z = -0x200;
			CIwMat scaleMat;

			scaleMat.SetIdentity();
			scaleMat.m[0][0] = (iwfixed)(scaleMat.m[0][0] * offset);
			scaleMat.m[1][1] = (iwfixed)(scaleMat.m[1][1] * offset);

			g_viewMatrix = viewMatrix.PreMult(scaleMat);
			setViewMatrix = true;
		}
	}
	else
	{
		g_animationPercent = (double)g_iAnimationIndex / 40; // take 2 seconds

		// Handle the rotation animation here
		double rotAngle = gScaledModeScaler->GetRotationAngle();
		perCentAngle = LiveMaps::RadToDeg(rotAngle) / 360;
		perCentAngle = perCentAngle * g_animationPercent;

		scaleX = 1 + ((gScaledModeScaler->GetScaleX() - 1) * g_animationPercent);
		scaleY = 1 + ((gScaledModeScaler->GetScaleY() - 1) * g_animationPercent);

		g_iAnimationIndex++;

		if (g_iAnimationIndex == 40)
		{
			g_bIsAnimating = false;
		}
		g_viewMatrix = gScaledModeScaler->GetDrawRotationMatrix(g_animationPercent);
		setViewMatrix = true;
	}

	if (setViewMatrix)
	{
		// Set our viewing point to the same as our field of view in Main.cpp -- IwGxSetPerspMul
		//g_viewMatrix.t.z = -0x200;

		//// Rotate the angle of our view matrix
		//iwangle degAng = (iwangle)(IW_GEOM_ONE * perCentAngle);

		//CIwMat rotZ;
		//rotZ.SetRotZ(degAng);
		//g_viewMatrix.CopyRot(rotZ);

		//CIwMat scaleMat;
		//scaleMat.SetIdentity();
		//scaleMat.m[0][0] = 4096 / scaleX;
		//scaleMat.m[1][1] = 4096 / scaleY;

		//g_viewMatrix = g_viewMatrix.PreMult(scaleMat);

		// Give a range of zooming for effect
		//IwGxSetFarZNearZ(0x400, 0x10);
		IwGxSetViewMatrix(&g_viewMatrix);
	}
	
	if (!g_bInitialLoad)
	{
		if (!g_bInProgress)
		{
			//if ((s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN) == 0)
			{
				MapTile* pNextDownload = GetNextDownload(NULL);
				if (pNextDownload)
				{
					g_bInProgress = true;
					LoadMapTileImage(pNextDownload);
				}
			}
		}
	}

    return true;
}

void MapBackground::RenderBackgroundOnSurface(CIw2DSurface* pSurface)
{
	std::list<MapTile*>::iterator iter = gVectorImageUrls.begin();

	// Set up a view matrix to rotate what we are viewing about the z-axis
	// This normalizes the center of the screen to (0,0), so we need to offset
	// our coordinates.
	// We also need to scale in our X and Y directions.

	CIwColour colClear;
	colClear = IwGxGetColClear();

	if (g_dAlpha < 0xFF)
	{
		IwGxSetColClear(0, 0, 0, 0);
	}

	CIwColour* cols = IW_GX_ALLOC(CIwColour, 4);
	for (int i = 0; i < 4; ++i)
	{
		cols[i].r = cols[i].g = cols[i].b = 0xff;
		cols[i].a = (uint8)g_dAlpha;
	}
	//static CIwSVec2 uvs[4] =
	//{
	//	CIwSVec2(0 << 12, 0 << 12),
	//	CIwSVec2(0 << 12, 1 << 12),
	//	CIwSVec2(1 << 12, 1 << 12),
	//	CIwSVec2(1 << 12, 0 << 12),
	//};
	static CIwSVec2 uvs[4] =
	{
		CIwSVec2((0 << 12) + 1, (0 << 12) + 1),
		CIwSVec2((0 << 12) + 1, (1 << 12) - 1),
		CIwSVec2((1 << 12) - 1, (1 << 12) - 1),
		CIwSVec2((1 << 12) - 1, (0 << 12) + 1),
	};

	static CIwSVec2 uvsRot[4] =
	{
		CIwSVec2((0 << 12) + 20, (0 << 12) + 20),
		CIwSVec2((0 << 12) + 20, (1 << 12) - 20),
		CIwSVec2((1 << 12) - 20, (1 << 12) - 20),
		CIwSVec2((1 << 12) - 20, (0 << 12) + 20),
	};

	while (iter != gVectorImageUrls.end())
	{
		MapTile* pTile = *iter;
		
		CIwTexture* pTexture = pTile->pTexture;

		if (pTexture)
		{
			//Calculate the top left of the map image
			CIwSVec2 topLeft = pTile->location;

			CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
			pMat->SetAlphaMode(CIwMaterial::ALPHA_BLEND);
			pMat->SetModulateMode(CIwMaterial::MODULATE_RGB);

			// Use Texture on Material
			pMat->SetTexture(pTexture);
			IwGxSetMaterial(pMat);

			int xOffset = IwGxGetScreenWidth() / 2;
			int yOffset = IwGxGetScreenHeight() / 2;

			CIwSVec3* pWSCoords= IW_GX_ALLOC(CIwSVec3, 4);
 			pWSCoords[0].x = topLeft.x; pWSCoords[0].y = topLeft.y;
			pWSCoords[1].x = topLeft.x; pWSCoords[1].y = topLeft.y + 256;
			pWSCoords[2].x = topLeft.x + 256; pWSCoords[2].y = topLeft.y + 256;
			pWSCoords[3].x = topLeft.x + 256; pWSCoords[3].y = topLeft.y;
			pWSCoords[0].z = pWSCoords[1].z = pWSCoords[2].z = pWSCoords[3].z = 0;

			// Scale the coordinates by offsetting, scaling and rendering
			for (int i = 0; i < 4; ++i)
			{
				pWSCoords[i].x -= xOffset;
				pWSCoords[i].y -= yOffset;
			}

			IwGxSetVertStreamWorldSpace(pWSCoords, 4);

			if (g_bScaledMode)
			{
				IwGxSetUVStream(uvsRot);
			}
			else
			{
				IwGxSetUVStream(uvs);
			}
			IwGxSetColStream(cols);
			IwGxDrawPrims(IW_GX_QUAD_LIST, NULL, 4);
		}
		iter++;
	}
	IwGxSetColStream(NULL);
	//IwGxSetColClear(colClear.r, colClear.g, colClear.b, colClear.a);
}

void MapBackground::RenderBackground()
{
	RenderBackgroundOnSurface(NULL);
}

//-----------------------------------------------------------------------------
// The following function displays the current time. 
//-----------------------------------------------------------------------------
void MapBackground::Render()
{
	RenderBackground();
}

