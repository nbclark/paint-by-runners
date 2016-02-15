#include "CreateMapGameState.h"
#include "tinyxml.h"
#include "MessageBox.h"
#include "TextEntryBox.h"

CreateMapGameState::CreateMapGameState(void)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "CreateMapGameState", CreateMapGameState, OnClickBack, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("createmap\\panel", "CIwUIElement");
	g_pDialogMain->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	CIwImage img, img2;
	img.LoadFromFile("cursor.png");
	gCursor = Iw2DCreateImage(img);

	img2.LoadFromFile("mark.png");
	gMark = Iw2DCreateImage(img2);

	g_location.x = g_location.y = 0;
	g_pBackground = new MapBackground;
	g_pBackground->Init();
}

CreateMapGameState::~CreateMapGameState(void)
{
	g_pBackground->ShutDown();
	delete g_pBackground;
	delete gCursor;
	delete gMark;
}

void CreateMapGameState::OnClickBack(CIwUIElement* Clicked)
{
	g_downPos.clear();
	g_pMapGame->SetGameState(GPS_GameState_SelectMap, AnimDir_Left);
}

bool hittest = false;
bool showHit = false;

void CreateMapGameState::PerformUpdate()
{
	// Allow for scrolling
	bool mouseClick = false;
	CIwSVec2 clickPos;

	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);

	if (g_tickCount == 1)
	{
		//g_location = g_pMapGame->GetPosition();
		g_tickCount = 0;
	}

	g_pBackground->UpdatePositionFromMouseAndKeyboard(clickPos, &mouseClick);

	showHit = false;
	static CIwSVec2 lastPos(-1, -1);
	if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN)
	{
		CIwSVec2 currentPos;

		currentPos.x = (int16)s3ePointerGetX();
		currentPos.y = (int16)s3ePointerGetY();

		if (hittest)
		{
			s3eLocation location;
			g_pBackground->GetScaler()->PositionToLocation(currentPos, &location);

			if (g_region.Contains(location))
			{
				showHit = true;
				s3eDeviceYield(0);
			}
			return;
		}

		if (lastPos.x != -1)
		{
			if (ABS(lastPos.x - currentPos.x) > 10 || ABS(lastPos.y - currentPos.y) > 10)
			{
				s3eLocation location;
				g_pBackground->GetScaler()->PositionToLocation(currentPos, &location);

				g_points.append(location);
				lastPos.x = currentPos.x;
				lastPos.y = currentPos.y;
			}
		}
		else
		{
			lastPos.x = currentPos.x;
			lastPos.y = currentPos.y;
		}
	}
	else if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_RELEASED)
	{
		if (g_points.size() > 5 && !hittest)
		{
			g_region.Clear();
			
			float maxLat = -10000, maxLon = -10000, minLat = 10000, minLon = 10000;
			for (int i = 0; i < g_points.size(); ++i)
			{
				maxLat = MAX(maxLat, g_points[i].m_Latitude);
				minLat = MIN(minLat, g_points[i].m_Latitude);
				maxLon = MAX(maxLon, g_points[i].m_Longitude);
				minLon = MIN(minLon, g_points[i].m_Longitude);
			}

			s3eLocation max,min;
			max.m_Latitude = maxLat;
			max.m_Longitude = maxLon;
			min.m_Latitude = minLat;
			min.m_Longitude = minLon;

			g_region.SetBoundingBox(min, max);

			s3eLocation prevLoc = g_points[0];
			for (int i = 1; i < g_points.size(); ++i)
			{
				g_region.Add(prevLoc, g_points[i]);
				prevLoc = g_points[i];
			}
			g_region.Add(prevLoc, g_points[0]);

			hittest = true;

			bool result = MessageBox::Show((char*)"Check Map", (char*)"Is the boundary displayed correctly?", (char*)"Yes", (char*)"No", CreateMapGameState::MessageRenderBackground, this);
			
			if (result)
			{
				TextEntryBox::Show((char*)"Save Map", (char*)"Please enter a tile for this boundary:", (char*)"Save Changes", CreateMapGameState::MessageRenderBackground, CreateMapGameState::TextEntryClosed, this);
			}
			else
			{
			}
			hittest = false;
			g_points.clear();
		}
	}
	else
	{
		lastPos.x = lastPos.y = -1;
	}
	g_pBackground->Update(false);
	/*
	else
	{
		if (g_downPos.size() == 4 && !g_pMapGame->GetBackground()->IsAnimating())
		{
			bool result = MessageBox::Show((char*)"Check Field", (char*)"Is the field displayed correct?", (char*)"Yes", (char*)"No", GameState::MessageRenderBackground, this);
			g_downPos.clear();
			
			if (result)
			{
				TextEntryBox::Show((char*)"Save Field", (char*)"Please enter a tile for this field:", (char*)"Save Changes", GameState::MessageRenderBackground, CreateMapGameState::TextEntryClosed, this);
			}
			else
			{
				this->g_pMapGame->GetBackground()->ClearScaledCorners();
			}
		}
		else
		{
			g_pMapGame->GetBackground()->Update(false);
		}
	}
	*/
}

void CreateMapGameState::MessageRenderBackground(void * pParam)
{
	CreateMapGameState* pThis = (CreateMapGameState*)pParam;
	pThis->RenderBackground();
	pThis->RenderClicks();
}

void CreateMapGameState::TextEntryClosed(void * pParam, const char* szText)
{
	CreateMapGameState* pGame = (CreateMapGameState*)pParam;
	// Prompt here for saving the thing

	char szPath[200];
	strcpy(szPath, "maps/");
	strcat(szPath, szText);
	strcat(szPath, ".map");

	double zoom = pGame->g_pBackground->GetScaler()->GetZoom();

	TiXmlDocument doc;

	TiXmlElement pKmlNode("kml");
	TiXmlElement pDocumentNode("Document");
	
	GPSRectangle scaledCorners = pGame->g_pBackground->GetScaler()->GetScaledCorners();

	for (int i = 0; i < pGame->g_points.size(); ++i)
	{
		s3eLocation location = pGame->g_points[i];

		TiXmlElement pPlacemarkNode("Placemark");
		TiXmlElement pPointNode("Point");
		TiXmlElement pCoordinatesNode("coordinates");
		
		char szValue[200];
		sprintf(szValue, "%f,%f", location.m_Latitude, location.m_Longitude);

		TiXmlText pValueNode(szValue);

		pCoordinatesNode.InsertEndChild(pValueNode);
		pPointNode.InsertEndChild(pCoordinatesNode);
		pPlacemarkNode.InsertEndChild(pPointNode);
		pDocumentNode.InsertEndChild(pPlacemarkNode);
	}

	char szZoom[20];
	sprintf(szZoom, "%f", zoom);
	pKmlNode.SetAttribute("zoom", szZoom);
	pKmlNode.InsertEndChild(pDocumentNode);
	doc.InsertEndChild(pKmlNode);
	doc.SaveFile(szPath);

	pGame->g_pMapGame->SetGameState(GPS_GameState_SelectMap, AnimDir_Left);
}

void CreateMapGameState::PerformRender()
{
	IwGxSetScreenSpaceSlot(-1);
	RenderBackground();
	IwGxFlush();
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();
	RenderClicks();
	RenderCursor();
}

void CreateMapGameState::RenderCursor()
{
	g_cursorIter++;
	CIwSVec2 topLeft;
	CIwVec2 topLeft1;

	double rotation = 0;

	if (g_cursorIter >= FRAMES_PER_S * 5.0)
	{
		g_cursorIter = 0;
		rotation = 0;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 3.5)
	{
		rotation = 315;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 3.0)
	{
		rotation = 270;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 2.5)
	{
		rotation = 225;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 2.0)
	{
		rotation = 180;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 1.5)
	{
		rotation = 135;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 1.0)
	{
		rotation = 90;
	}
	else if (g_cursorIter >= FRAMES_PER_S * 0.5)
	{
		rotation = 45;
	}
	else
	{
		rotation = 0;
	}

	rotation = ((g_cursorIter/5) % 10) * 36;
	
	CIwMat2D rot;
	CIwVec2 vec2;
	CIwSVec2 pos;

	int imageSize = gCursor->GetWidth() / 2;
	int width = Iw2DGetSurfaceWidth();
	int height = Iw2DGetSurfaceHeight();

	pos.x = g_location.x - imageSize;
	pos.y = g_location.y - imageSize;

	vec2.x = g_location.x;
	vec2.y = g_location.y;

	if (!(vec2.x < -imageSize || vec2.y < -imageSize || vec2.x > (imageSize+width) || vec2.y > (imageSize+height)))
	{
		rot.SetRot((iwangle)(IW_GEOM_ONE * rotation / 360), vec2);
		Iw2DSetTransformMatrix(rot);
		Iw2DDrawImage(gCursor, pos);
		Iw2DSetTransformMatrix(CIwMat2D::g_Identity);
	}
	
	g_cursorIter++;
}

void CreateMapGameState::RenderBackground()
{
	g_pBackground->Render();
}

void CreateMapGameState::RenderClicks()
{
	std::list<s3eLocation>::iterator downiter = g_downPos.begin();
	CIwSVec2 vec;
	CIwVec2 vec2;
	s3eLocation location;

	CIwMat2D rot;

	int size = gMark->GetWidth() / 2;
	int width = Iw2DGetSurfaceWidth();
	int height = Iw2DGetSurfaceHeight();

	if (hittest)
	{
		CIwSVec2 currentPos;
		currentPos.x = (int16)s3ePointerGetX();
		currentPos.y = (int16)s3ePointerGetY();

		s3eLocation location;
		g_pBackground->GetScaler()->PositionToLocation(currentPos, &location);

		Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
		Iw2DSetColour(0xFFFFFFFF);
		for (int i = 0; i < g_region.g_triangles.size(); ++i)
		{
			CIwSVec2 A,B,C;
			g_pBackground->GetScaler()->LocationToPosition(g_region.g_triangles[i].A, &A);
			g_pBackground->GetScaler()->LocationToPosition(g_region.g_triangles[i].B, &B);
			g_pBackground->GetScaler()->LocationToPosition(g_region.g_triangles[i].C, &C);
			
			//Iw2DDrawLine(A,B);
			//Iw2DDrawLine(B,C);
			//Iw2DDrawLine(C,A);

			//if (s3ePointerGetState(S3E_POINTER_BUTTON_SELECT) & S3E_POINTER_STATE_DOWN)
			{
					//if (g_region.g_triangles[i].Contains(location))
					{
						CIwSVec2 verts[3];
						verts[0] = A;
						verts[1] = B;
						verts[2] = C;
						Iw2DFillPolygon(verts, 3);
					}
			}
		}
		Iw2DFinishDrawing();
	}
	else if (g_points.size() > 1)
	{
		Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
		Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);
		Iw2DSetColour(0xFFFFFFFF);

		CIwSVec2 prevLoc;

		g_pBackground->GetScaler()->LocationToPosition(g_points[0], &prevLoc);

		for (int i = 1; i < g_points.size() && i < 100; ++i)
		{
			location = g_points[i];
			g_pBackground->GetScaler()->LocationToPosition(location, &vec);
			
			//if (!(vec.x < -size || vec.y < -size || vec.x > (size+width) || vec.y > (size+height)))
			{
				float diffX = (float)(vec.x - prevLoc.x) / 10;
				float diffY = (float)(vec.y - prevLoc.y) / 10;

				for (int i = 0; i < 10; ++i)
				{
					CIwSVec2 loc(prevLoc.x + diffX*i, prevLoc.y + diffY*i);
					Iw2DFillArc(loc, CIwSVec2(8, 8), 0, IW_ANGLE_PI * 2, 0);
				}
				prevLoc = vec;
			}
		}
		/*
		for (int i = 1; i < g_points.size() && i < 100; ++i)
		{
			location = g_points[i];
			g_pBackground->GetScaler()->LocationToPosition(location, &vec);
			
			CIwSVec2 vec2 = vec - CIwSVec2(size, size);
			Iw2DDrawImage(gMark, vec2);
		}
		*/
	}
	Iw2DSetTransformMatrix(CIwMat2D::g_Identity);
}

void CreateMapGameState::PerformActivate()
{
	g_cursorIter = 0;
	
	s3eLocation loc;
	
	Utils::GetLocation(&loc);
	g_pBackground->SetLocation(loc.m_Longitude, loc.m_Latitude);
	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);

	g_region.Clear();
	g_points.clear();
}

void CreateMapGameState::PerformDeActivate()
{
}