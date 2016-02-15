#include "SelectMapGameState.h"
#include "tinyxml.h"
#include "MessageBox.h"

SelectMapGameState::SelectMapGameState(void)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectMapGameState", SelectMapGameState, OnClickMapItem, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectMapGameState", SelectMapGameState, OnClickDelete, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectMapGameState", SelectMapGameState, OnClickBack, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("selectmap\\panel", "CIwUIElement");
	g_pDialogMain->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);
	g_firstSelectShow = true;
	g_szDeleteFile = NULL;

	g_pBackground = (CIwTexture*)IwGetResManager()->GetResNamed("fullback", "CIwTexture"); 
}

SelectMapGameState::~SelectMapGameState(void)
{
	ClearLoadedMaps(false);
	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void SelectMapGameState::ClearLoadedMaps(bool removeButtons)
{
	if (removeButtons)
	{
		CIwUIElement* pItems = g_pDialogMain->GetChildNamed("Maps");
		std::list<CIwUIElement*>::iterator iter = g_savedMaps.begin();

		while (iter != g_savedMaps.end())
		{
			pItems->RemoveChild(*iter);
			delete *iter;
			iter++;
		}
		g_savedMaps.clear();
	}
	std::list<ButtonHash*>::iterator iter2 = g_loadedMaps.begin();

	while (iter2 != g_loadedMaps.end())
	{
		delete[] (*iter2)->szPath;
		delete *iter2;
		iter2++;
	}
	g_loadedMaps.clear();
}

void SelectMapGameState::PerformUpdate()
{
	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);

	if (NULL != g_szDeleteFile)
	{
		bool result = MessageBox::Show((char*)"Delete Map", (char*)"Are you sure you want to delete this map?", (char*)"Yes", (char*)"No", GameState::MessageRenderBackground, this);
		
		if (result)
		{
			char* szDelete = this->g_szDeleteFile;
			s3eFileDelete(szDelete);
			this->Activate();
		}
		g_szDeleteFile = NULL;
	}
}

void SelectMapGameState::PerformRender()
{
	IwGxSetScreenSpaceSlot(-1);
	
	CIwSVec2 backLoc(0,0);
	Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

	IwGxFlush();
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();
}

void SelectMapGameState::PerformActivate()
{
	CIwUIElement* pItems = g_pDialogMain->GetChildNamed("Maps");
	CIwUIElement* pButtonTemplate = (CIwUIElement*)IwGetResManager()->GetResNamed("ButtonTemplate", "CIwUIElement");

	if (!g_firstSelectShow)
	{
		ClearLoadedMaps(true);
	}
	else
	{
		for (int i = 0; i < 2; ++i)
		{
			const char *szText = (i == 0) ? "Create New Field..." : "Load Tilt Field...";
			const char *szName = (i == 0) ? "CreateMapButton" : "LoadTiltButton";

			CIwUIElement* pTemplateNew = (CIwUIElement*)pButtonTemplate->Clone();
			CIwUIButton* pButtonNew = (CIwUIButton*)pTemplateNew->GetChildNamed("ButtonBase");
			CIwUILabel* pLabelNew = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Text", true, false);
			CIwUILabel* pLabelDistance = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Distance", true, false);
			pButtonNew->GetChildNamed("DeleteButton")->SetVisible(false);

			pLabelNew->SetCaption(szText);
			pLabelDistance->SetVisible(false);
			pTemplateNew->SetName(szName);
			pItems->GetLayout()->AddElement(pTemplateNew);

			if (i == 0)
			{
				g_pCreateButton = pButtonNew;
			}
			else
			{
				g_pTiltButton = pButtonNew;
			}
		}

		g_firstSelectShow = false;
	}

	CIwUIElement* pFocusTemplate = pItems->GetChildNamed("CreateMapButton")->GetChildNamed("ButtonBase");
	s3eFileList* pFiles = s3eFileListDirectory("maps");

	if (pFiles)
	{
		char szFile[200], szName[200], szPath[200];
		char* szLoadedMap = "";//g_pMapGame->GetLoadedMap();

		CIwArray<s3eLocation> corners;
		s3eLocation currLoc;
		
		Utils::GetLocation(&currLoc);

		while (S3E_RESULT_SUCCESS == s3eFileListNext(pFiles, szFile, 200))
		{
			if (strstr(szFile, ".map"))
			{
				CIwUIElement* pTemplate = (CIwUIElement*)pButtonTemplate->Clone();
				CIwUIElement* pButton = (CIwUIButton*)pTemplate->GetChildNamed("ButtonBase");
				CIwUILabel* pLabel = (CIwUILabel*)pButton->GetChildNamed("ButtonTemplate_Text", true, false);
				CIwUILabel* pLabelDist = (CIwUILabel*)pButton->GetChildNamed("ButtonTemplate_Distance", true, false);

				strcpy(szName, szFile);
				strrchr(szName, '.')[0] = 0;

				strcpy(szPath, "maps\\");
				strcat(szPath, szFile);

				pLabel->SetCaption(szName);
				//pButton->AddHashString(0, szPath);
				char buf[200];
				sprintf(buf, "item_%s", szFile);
				pTemplate->SetName(buf);

				if (0 == stricmp(szLoadedMap, szPath))
				{
					pFocusTemplate = pButton;
				}

				char szDistance[100];
				float zoom;
				LoadMap(szPath, &corners, &zoom);
				//s3eLocation center = LiveMaps::CalculateCenter(corners);

				double distanceInMeters = LiveMaps::CalculateDistance(corners[0], currLoc);
				double distanceInMiles = MilesPerMeter * distanceInMeters;

				sprintf(szDistance, "%.2f miles away", distanceInMiles);
				pLabelDist->SetCaption(szDistance);

				int len = strlen(szPath);
				ButtonHash* pHash = new ButtonHash;
				pHash->pTemplate = pTemplate;
				pHash->pElement = pButton;
				pHash->fDistance = (float)distanceInMiles;
				pHash->szPath = new char[len+1];
				strcpy(pHash->szPath, szPath);

				g_loadedMaps.push_back(pHash);
				g_savedMaps.push_back(pTemplate);
			}
		}
		s3eFileListClose(pFiles);
	}

	g_loadedMaps.sort(ButtonHashSorter());
	std::list<ButtonHash*>::iterator iter = g_loadedMaps.begin();

	while (iter != g_loadedMaps.end())
	{
		pItems->GetLayout()->AddElement((*iter)->pTemplate);
		iter++;
	}

	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);
	IwGetUIView()->RequestFocus(pFocusTemplate);
}

void SelectMapGameState::PerformDeActivate()
{
	s3eDeviceYield(0);
}

void SelectMapGameState::OnClickBack(CIwUIElement* Clicked)
{
	g_pMapGame->SetGameState(GPS_GameState_SelectGame, AnimDir_Left);
}

void SelectMapGameState::OnClickDelete(CIwUIElement* Clicked)
{
	char* fileName = NULL;

	Clicked = Clicked->GetParent();

	std::list<ButtonHash*>::iterator iter = g_loadedMaps.begin();

	while (iter != g_loadedMaps.end())
	{
		if ((*iter)->pElement == Clicked)
		{
			fileName = (*iter)->szPath;
		}
		iter++;
	}

	if (NULL != fileName)
	{
		g_szDeleteFile = fileName;
	}
	else
	{
		g_szDeleteFile = NULL;
	}
}

void SelectMapGameState::OnClickMapItem(CIwUIElement* Clicked)
{
	char* fileName = NULL;

	if ((CIwUIButton*)Clicked == g_pCreateButton)
	{
		g_pMapGame->SetGameState(GPS_GameState_CreateMap, AnimDir_Right);
	}
	else
	{
		std::list<ButtonHash*>::iterator iter = g_loadedMaps.begin();

		while (iter != g_loadedMaps.end())
		{
			if ((*iter)->pElement == Clicked)
			{
				fileName = (*iter)->szPath;
			}
			iter++;
		}

		if (NULL == fileName || !strcmp(fileName, ""))
		{
			g_pMapGame->SetGameState(GPS_GameState_CreateMap, AnimDir_Right);
		}
		else
		{
			bool saved = false;
			g_loadedCorners.clear();

			float zoom;
			if (LoadMap(fileName, &g_loadedCorners, &zoom))
			{
				Region r;
				Utils::LoadRegion(r, &g_loadedCorners);

				g_pMapGame->SetBoundingRegion(fileName, &r);
				g_pMapGame->SetGameState(GPS_GameState_SelectGame, AnimDir_Left);
			}
		}
	}
}

bool SelectMapGameState::LoadMap(char* szMap, CIwArray<s3eLocation>* pPoints, float* pZoom)
{
	pPoints->clear();
	s3eFile* pFile = s3eFileOpen(szMap, "r");

	if (pFile)
	{
		uint32 gResultLen = s3eFileGetSize(pFile);
		char* gResult = (char*)s3eMalloc(gResultLen + 1);

		s3eFileRead(gResult, sizeof(char), gResultLen, pFile);

		bool result = Utils::LoadMap(gResult, pPoints, pZoom);

		s3eFree(gResult);
		s3eFileClose(pFile);

		return result;
	}
	return false;
}