#include "HighScoreGameState.h"
#include "tinyxml.h"
#include "MessageBox.h"

HighScoreGameState::HighScoreGameState(void)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "HighScoreGameState", HighScoreGameState, OnClickBack, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("highscore\\panel", "CIwUIElement");
	g_pDialogMain->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	g_iHighScores = 0;
}

HighScoreGameState::~HighScoreGameState(void)
{
	ClearScores(false);
	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void HighScoreGameState::ClearScores(bool removeButtons)
{
	if (removeButtons)
	{
		CIwUIElement* pItems = g_pDialogMain->GetChildNamed("Scores");
		std::list<CIwUIElement*>::iterator iter = g_savedMaps.begin();

		while (iter != g_savedMaps.end())
		{
			pItems->RemoveChild(*iter);
			delete *iter;
			iter++;
		}
		g_savedMaps.clear();
	}
}

void HighScoreGameState::PerformUpdate()
{
	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);
}

void HighScoreGameState::PerformRender()
{
	IwGxSetScreenSpaceSlot(-1);
	IwGxFlush();
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();
}

void HighScoreGameState::AddScore(int newScore, uint64 newTime, double newDistance)
{
	this->LoadScores(newScore, newTime, newDistance, true);
}

void HighScoreGameState::LoadScores(int newScore, uint64 newTime, double newDistance, bool addScore)
{
	g_iHighScores = 0;

	bool needSave = false;
 	char* szHighScore = "picnicgames.score"; //g_pMapGame->GetGameEngine()->GetHighScoreFile();
	if (szHighScore)
	{
		char szFile[100];
		strcpy(szFile, "scores/");
		strcat(szFile, szHighScore);

		TiXmlDocument doc;
		doc.LoadFile(szFile);
		TiXmlElement* pRootNode = doc.RootElement();
		TiXmlNode* pScoreNode;

		if (pRootNode && pRootNode->ToElement())
		{
			for (pScoreNode = pRootNode->FirstChild("Score"); pScoreNode; pScoreNode = pScoreNode->NextSibling("Score"))
			{
				const char* szTime = pScoreNode->FirstChild("Time")->FirstChild()->Value();
				const char* szScore = pScoreNode->FirstChild("Score")->FirstChild()->Value();
				const char* szDistance = pScoreNode->FirstChild("Distance")->FirstChild()->Value();

				int score = atoi(szScore);
				uint64 time = atol(szTime);
				double distance = atof(szDistance);

				if (addScore && newScore > score)
				{
					if (g_iHighScores >= MAX_HIGHSCORES)
					{
						break;
					}
					addScore = false;
					needSave = true;

					g_highScores[g_iHighScores].Score = newScore;
					g_highScores[g_iHighScores].Time = newTime;
					g_highScores[g_iHighScores].DistanceTravelled = newDistance;

					g_iHighScores++;
				}

				if (g_iHighScores >= MAX_HIGHSCORES)
				{
					break;
				}
				g_highScores[g_iHighScores].Score = score;
				g_highScores[g_iHighScores].Time = time;
				g_highScores[g_iHighScores].DistanceTravelled = distance;

				g_iHighScores++;
			}
		}
		if (addScore)
		{
			g_highScores[g_iHighScores].Score = newScore;
			g_highScores[g_iHighScores].Time = newTime;
			g_highScores[g_iHighScores].DistanceTravelled = newDistance;
			g_iHighScores++;

			needSave = true;
		}
	}
	if (needSave)
	{
		SaveScores();
	}
}

void HighScoreGameState::SaveScores()
{
 	char* szHighScore = "picnicgames.score"; //g_pMapGame->GetGameEngine()->GetHighScoreFile();
	if (szHighScore)
	{
		char szFile[100];
		strcpy(szFile, "scores/");
		strcat(szFile, szHighScore);
		
		TiXmlElement pScoresNode("Scores");

		for (int i = 0; i < g_iHighScores; ++i)
		{
			TiXmlElement pScoreNode("Score");

			char szScoreValue[20];
			char szTimeValue[20];
			char szDistanceValue[20];

			TiXmlElement pScoreItemNode("Score");
			TiXmlElement pTimeItemNode("Time");
			TiXmlElement pDistanceItemNode("Distance");

			sprintf(szScoreValue, "%d", g_highScores[i].Score);
			TiXmlText pScoreValueNode(szScoreValue);

			sprintf(szTimeValue, "%d", g_highScores[i].Time);
			TiXmlText pTimeValueNode(szTimeValue);

			sprintf(szDistanceValue, "%5.2f", g_highScores[i].DistanceTravelled);
			TiXmlText pDistanceValueNode(szDistanceValue);

			pScoreItemNode.InsertEndChild(pScoreValueNode);
			pTimeItemNode.InsertEndChild(pTimeValueNode);
			pDistanceItemNode.InsertEndChild(pDistanceValueNode);
			
			pScoreNode.InsertEndChild(pScoreItemNode);
			pScoreNode.InsertEndChild(pTimeItemNode);
			pScoreNode.InsertEndChild(pDistanceItemNode);
			pScoresNode.InsertEndChild(pScoreNode);
		}

		TiXmlDocument doc;
		doc.InsertEndChild(pScoresNode);
		doc.SaveFile(szFile);
	}
}

void HighScoreGameState::PerformActivate()
{
	ClearScores(true);

	CIwUIElement* pItems = g_pDialogMain->GetChildNamed("Scores");
	CIwUIElement* pButtonTemplate = (CIwUIElement*)IwGetResManager()->GetResNamed("ScoreTemplate", "CIwUIElement");

	LoadScores(0, 0, 0, false);

	for (int i = 0; i < g_iHighScores; ++i)
	{
		CIwUIElement* pTemplate = (CIwUIElement*)pButtonTemplate->Clone();
		CIwUIElement* pButton = (CIwUIButton*)pTemplate->GetChildNamed("ButtonBase");
		CIwUILabel* pIndex = (CIwUILabel*)pButton->GetChildNamed("ButtonTemplate_Index", true, false);
		CIwUILabel* pLabel = (CIwUILabel*)pButton->GetChildNamed("ButtonTemplate_Text", true, false);
		CIwUILabel* pLabelTime = (CIwUILabel*)pButton->GetChildNamed("ButtonTemplate_Time", true, false);
		CIwUILabel* pLabelDist = (CIwUILabel*)pButton->GetChildNamed("ButtonTemplate_Distance", true, false);

		char szName[10];

		sprintf(szName, "%d", i);
		pTemplate->SetName(szName);

		char szScore[20];
		sprintf(szScore, "%d.", (i+1));
		pIndex->SetCaption(szScore);

		sprintf(szScore, "%d", g_highScores[i].Score);
		pLabel->SetCaption(szScore);

		char szDistance[40];

		// convert meters to feet
		sprintf(szDistance, "%5.2f feet travelled", g_highScores[i].DistanceTravelled * 3.2808399);
		pLabelDist->SetCaption(szDistance);

		int minutes = (int)(g_highScores[i].Time / (60000));
		int seconds = (int)((g_highScores[i].Time / 1000) % (60));

		sprintf(szDistance, "%d min, %d sec", minutes, seconds);
		pLabelTime->SetCaption(szDistance);

		g_savedMaps.push_back(pTemplate);
		pItems->GetLayout()->AddElement(pTemplate);
	}

	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);
}

void HighScoreGameState::PerformDeActivate()
{
}

void HighScoreGameState::OnClickBack(CIwUIElement* Clicked)
{
	g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Left);
}