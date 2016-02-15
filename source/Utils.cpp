#include "Utils.h"
#include "tinyxml.h"
#include "IwJPEG.h"
#include "IwHttpQueue.h"
#include "IwMultiPlayerHandler.h"
#include "IwNotificationHandler.h"

char g_szMapData[10000];
float Utils::g_latOffset, Utils::g_lonOffset;
uint64 Utils::g_lastGpsRead = 0;
s3eLocation Utils::g_lastGpsCoord;

void Utils::GotTile(void* Argument, const char* szContentType, const char * Result, uint32 ResultLen)
{
	CIwTexture** ppTile = (CIwTexture**)Argument;
	CIwImage image;
	JPEGImage((void*)Result, ResultLen, image);
	*ppTile = new CIwTexture;
	(*ppTile)->CopyFromImage(&image);
	(*ppTile)->Upload();
}

void Utils::GotTileError(void * Argument)
{
	CIwTexture** ppTile = (CIwTexture**)Argument;
	(*ppTile) = new CIwTexture;
	(*ppTile)->LoadFromFile("notfound.png");
	(*ppTile)->Upload();
}

void Utils::DownloadMapTile(CIwTexture** ppTile, char* szImageUrl)
{
	IwGetHTTPQueue()->Get(szImageUrl, ppTile, GotTile, GotTileError);
	
	static int counter = 0;
	
	CIwTexture* texx = (CIwTexture*)IwGetResManager()->GetResNamed("logo", IW_GX_RESTYPE_TEXTURE);
	uint32 w1 = texx->GetWidth();
	uint32 h1 = texx->GetHeight();

	static CIwSVec2 uvs[4] =
	{
		CIwSVec2((0 << 12) + 10, (0 << 12) + 10),
		CIwSVec2((0 << 12) + 10, (1 << 12) - 10),
		CIwSVec2((1 << 12) - 10, (1 << 12) - 10),
		CIwSVec2((1 << 12) - 10, (0 << 12) + 10),
	};
	
	int w = w1/2;
	int h = h1/2;
	while (!*ppTile)
	{
		IwGetHTTPQueue()->Update();
		IwGetNotificationHandler()->Update();
		IwGetMultiplayerHandler()->Update();

		IwGxSetColClear(0, 0, 0, 255);
		IwGxClear(IW_GX_COLOUR_BUFFER_F | IW_GX_DEPTH_BUFFER_F);

		CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
		pMat->SetModulateMode(CIwMaterial::MODULATE_NONE);
		pMat->SetTexture(texx);
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

		IwGxSetVertStreamWorldSpace(pWSCoords, 4);
		IwGxSetUVStream(uvs);
		IwGxDrawPrims(IW_GX_QUAD_LIST, NULL, 4);
		IwGetNotificationHandler()->Render();

		IwGxFlush();
		IwGxSwapBuffers();
		s3eDeviceYield(50);

		counter = (counter + 1) % 160;
	}

	CIwMat clearMat;
	clearMat.SetIdentity();
	IwGxSetViewMatrix(&clearMat);
}

char* Utils::GetMapData(const char* szMap)
{
	s3eFile* pFile = s3eFileOpen(szMap, "r");

	if (pFile)
	{
		uint32 gResultLen = s3eFileGetSize(pFile);

		IwAssert("FF", gResultLen < sizeof(g_szMapData));
		s3eFileRead(g_szMapData, sizeof(char), gResultLen, pFile);

		s3eFileClose(pFile);
		return &g_szMapData[0];
	}

	return NULL;
}

void Utils::SaveMapData(const char* szMap, const char* szData)
{
	s3eFile* pFile = s3eFileOpen(szMap, "w");
	s3eFileWrite(szData, sizeof(char), strlen(szData)+1, pFile);
	s3eFileClose(pFile);
}

void Utils::LoadRegion(Region& r, CIwArray<s3eLocation>* pPoints)
{
	CIwArray<s3eLocation> points = *pPoints;
	float maxLat = -10000, maxLon = -10000, minLat = 10000, minLon = 10000;
	for (int i = 0; i < pPoints->size(); ++i)
	{
		maxLat = MAX(maxLat, points[i].m_Latitude);
		minLat = MIN(minLat, points[i].m_Latitude);
		maxLon = MAX(maxLon, points[i].m_Longitude);
		minLon = MIN(minLon, points[i].m_Longitude);
	}

	s3eLocation max,min;
	max.m_Latitude = maxLat;
	max.m_Longitude = maxLon;
	min.m_Latitude = minLat;
	min.m_Longitude = minLon;

	r.Clear();
	r.SetBoundingBox(min, max);

	s3eLocation prevLoc = points[0];
	for (int i = 1; i < pPoints->size(); ++i)
	{
		r.Add(prevLoc, points[i]);
		prevLoc = points[i];
	}
	r.Add(prevLoc, points[0]);
}

bool Utils::LoadMap(const char* szData, CIwArray<s3eLocation>* pPoints, float* pZoom)
{
	pPoints->clear();

	TiXmlDocument doc;
	doc.Parse(szData);

	TiXmlElement* pRootNode = doc.RootElement();
	TiXmlNode* pDocumentNode;
	TiXmlNode* pPlacemarkNode;
	TiXmlNode* pPointNode;
	TiXmlNode* pCoordinateNode;

	if (pRootNode && pRootNode->ToElement())
	{
		pDocumentNode = pRootNode->FirstChild("Document");

		if (pDocumentNode && pDocumentNode->ToElement())
		{
			if (pZoom)
			{
				TiXmlAttribute* pAttribute = pRootNode->FirstAttribute();
				if (pAttribute)
				{
					*pZoom = (float)pAttribute->DoubleValue();
				}
				else
				{
					*pZoom = 19;
				}
			}
			for (pPlacemarkNode = pDocumentNode->FirstChild("Placemark"); pPlacemarkNode; pPlacemarkNode = pPlacemarkNode->NextSibling("Placemark"))
			{
				pPointNode = pPlacemarkNode->FirstChild("Point");

				if (pPointNode)
				{
					pCoordinateNode = pPointNode->FirstChild("coordinates");

					if (pCoordinateNode)
					{
						const char* szData = pCoordinateNode->FirstChild()->Value();

						if (strstr(szData, ","))
						{
							// we have coordinates
							float latitude, longitude;
							if (2 == sscanf(szData, "%f,%f", &latitude, &longitude))
							{
								// this one is good
								s3eLocation location;
								location.m_Latitude = latitude;
								location.m_Longitude = longitude;

								pPoints->push_back(location);
							}
						}
					}
				}
			}
			return true;
		}
	}
	return false;
}

void Utils::AlphaRenderImage(CIwTexture* pTexture, CIwSVec2& location, double alpha)
{
	int width = pTexture->GetWidth();
	int height = pTexture->GetHeight();

	CIwRect rect(location.x, location.y, width, height);
	Utils::AlphaRenderImage(pTexture, rect, alpha);
}

void Utils::AlphaRenderImage(CIwTexture* pTexture, CIwSVec2& location, double alpha, DrawPrimsDelegate pfnDrawPrims)
{
	int width = pTexture->GetWidth();
	int height = pTexture->GetHeight();

	CIwRect rect(location.x, location.y, width, height);
	Utils::AlphaRenderImage(pTexture, rect, alpha, pfnDrawPrims);
}

void Utils::DrawPrimsQuadList(CIwSVec2* pCoords)
{
	IwGxDrawPrims(IW_GX_QUAD_LIST, NULL, 4);
}

void Utils::AlphaRenderImage(CIwTexture* pTexture, CIwRect& bounds, double alpha)
{
	AlphaRenderImage(pTexture, bounds, alpha, DrawPrimsQuadList);
}

void Utils::AlphaRenderImage(CIwTexture* pTexture, CIwRect& bounds, double alpha, DrawPrimsDelegate pfnDrawPrims)
{
	int xOffset = IwGxGetScreenWidth() / 2;
	int yOffset = IwGxGetScreenHeight() / 2;

	int width = bounds.w << 3;
	int height = bounds.h << 3;
	
	CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
	pMat->SetAlphaMode(CIwMaterial::ALPHA_BLEND);
	pMat->SetModulateMode(CIwMaterial::MODULATE_RGB);

	// Use Texture on Material
	pMat->SetTexture(pTexture);
	IwGxSetMaterial(pMat);

	int wo2 = bounds.x << 3;
	int ho2 = bounds.y << 3;

	CIwSVec3* pWSCoords= IW_GX_ALLOC(CIwSVec3, 4);
	pWSCoords[0].x = wo2; pWSCoords[0].y = ho2;
	pWSCoords[1].x = wo2; pWSCoords[1].y = ho2 + height;
	pWSCoords[2].x = wo2 + width; pWSCoords[2].y = ho2 + height;
	pWSCoords[3].x = wo2 + width; pWSCoords[3].y = ho2;
	pWSCoords[0].z = pWSCoords[1].z = pWSCoords[2].z = pWSCoords[3].z = 0;

	CIwSVec2* pWSCoords2= IW_GX_ALLOC(CIwSVec2, 4);
	pWSCoords2[0].x = wo2;
	pWSCoords2[0].y = ho2;
	pWSCoords2[1].x = wo2;
	pWSCoords2[1].y = ho2 + height;
	pWSCoords2[2].x = wo2 + width;
	pWSCoords2[2].y = ho2 + height;
	pWSCoords2[3].x = wo2 + width;
	pWSCoords2[3].y = ho2;

	IwGxSetVertStreamScreenSpaceSubPixel(pWSCoords2, 4);

	static CIwSVec2 uvs[4] =
	{
		CIwSVec2((0 << 12) + 0, (0 << 12) + 0),
		CIwSVec2((0 << 12) + 0, (1 << 12) - 0),
		CIwSVec2((1 << 12) - 0, (1 << 12) - 0),
		CIwSVec2((1 << 12) - 0, (0 << 12) + 0),
	};

	CIwColour* cols = IW_GX_ALLOC(CIwColour, 4);
	for (int i = 0; i < 4; ++i)
	{
		cols[i].r = cols[i].g = cols[i].b = 0xff;
		cols[i].a = (uint8)alpha;
	}

	IwGxSetColStream(cols);
	IwGxSetUVStream(uvs);

	pfnDrawPrims(pWSCoords2);
}

void Utils::AlphaRenderAndRotateImage(CIwTexture* pTexture, CIwRect& bounds, double alpha, double rotationAngle)
{
	int xOffset = IwGxGetScreenWidth() / 2;
	int yOffset = IwGxGetScreenHeight() / 2;

	int width = bounds.w;// << 3;
	int height = bounds.h;// << 3;
	
	CIwMaterial* pMat = IW_GX_ALLOC_MATERIAL();
	pMat->SetAlphaMode(CIwMaterial::ALPHA_BLEND);
	pMat->SetModulateMode(CIwMaterial::MODULATE_RGB);

	// Use Texture on Material
	pMat->SetTexture(pTexture);
	IwGxSetMaterial(pMat);

	int wo2 = width / 2;// << 3;
	int ho2 = height / 2;// << 3;

	CIwSVec3* pWSCoords= IW_GX_ALLOC(CIwSVec3, 4);
	pWSCoords[0].x = -wo2; pWSCoords[0].y = -ho2;
	pWSCoords[1].x = -wo2; pWSCoords[1].y = -ho2 + height;
	pWSCoords[2].x = -wo2 + width; pWSCoords[2].y = -ho2 + height;
	pWSCoords[3].x = -wo2 + width; pWSCoords[3].y = -ho2;
	pWSCoords[0].z = pWSCoords[1].z = pWSCoords[2].z = pWSCoords[3].z = 0;

	CIwSVec2* pWSCoords2= IW_GX_ALLOC(CIwSVec2, 4);
	pWSCoords2[0].x = wo2; pWSCoords2[0].y = ho2;
	pWSCoords2[1].x = wo2; pWSCoords2[1].y = ho2 + height;
	pWSCoords2[2].x = wo2 + width; pWSCoords2[2].y = ho2 + height;
	pWSCoords2[3].x = wo2 + width; pWSCoords2[3].y = ho2;

	CIwMat viewMat = IwGxGetViewMatrix();

	CIwMat test = viewMat;
	test.t.x = -(bounds.x + wo2 - xOffset);
	test.t.y = -(bounds.y + ho2 - yOffset);

	IwGxSetViewMatrix(&test);

	CIwMat modelMat;
	modelMat.SetIdentity();
	modelMat.t.x = 0;
	modelMat.t.y = 0;

	iwangle degAng = (iwangle)(IW_GEOM_ONE * rotationAngle);

	CIwMat rotZ;
	rotZ.SetRotZ(degAng);

	modelMat.CopyRot(rotZ);

	IwGxSetModelMatrix(&modelMat);
	IwGxSetVertStreamModelSpace(pWSCoords, 4);

	static CIwSVec2 uvs[4] =
	{
		CIwSVec2((0 << 12) + 50, (0 << 12) + 50),
		CIwSVec2((0 << 12) + 50, (1 << 12) - 50),
		CIwSVec2((1 << 12) - 50, (1 << 12) - 50),
		CIwSVec2((1 << 12) - 50, (0 << 12) + 50),
	};

	//uvs[1].y = pTexture->m_UVScale.y - 1;
	//uvs[2].x = pTexture->m_UVScale.x - 1;
	//uvs[2].y = pTexture->m_UVScale.y - 1;
	//uvs[3].x = pTexture->m_UVScale.x - 1;

	CIwColour* cols = IW_GX_ALLOC(CIwColour, 4);
	for (int i = 0; i < 4; ++i)
	{
		cols[i].r = cols[i].g = cols[i].b = 0xff;
		cols[i].a = (uint8)alpha;
	}

	IwGxSetUVStream(uvs);
	IwGxSetColStream(cols);
	IwGxDrawPrims(IW_GX_QUAD_LIST, NULL, 4);

	IwGxSetViewMatrix(&viewMat);
}