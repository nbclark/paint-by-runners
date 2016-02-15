#include "PatteRunGameEngine.h"
#include "MessageBox.h"

PatteRunGameEngine::PatteRunGameEngine()
{
}
void PatteRunGameEngine::Init(void* pGameStateVoid)
{
	IGameHandler* pGameState = (IGameHandler*)pGameStateVoid;
	/*
	g_pBall = new CBall(pGameState);
	g_pUserPaddle = new CGPSPaddle(pGameState);
	//g_pUserPaddle = new CCompPaddle(this, g_pBall, false);
	g_pCompPaddle = new CCompPaddle(pGameState, g_pBall, true);

	g_pBall->PushPaddle(g_pUserPaddle);
	g_pBall->PushPaddle(g_pCompPaddle);
	*/
	g_pUser = new CGPSUser(pGameState);
	g_pGameHandler = pGameState;
	g_pFontHuge = (CIwGxFont*)IwGetResManager()->GetResNamed("font_huge", "CIwGxFont");
	g_pFont = (CIwGxFont*)IwGetResManager()->GetResNamed("font_medium", "CIwGxFont");
	g_pFontSmall = (CIwGxFont*)IwGetResManager()->GetResNamed("font_small", "CIwGxFont");
	
	CIwImage imgTile;
	imgTile.LoadFromFile("images/patterun/tile.png");

	g_pTile = new CIwTexture();
	g_pTile->CopyFromImage(&imgTile);
	g_pTile->Upload();
}

PatteRunGameEngine::~PatteRunGameEngine(void)
{
	/*
	delete g_pBall;
	delete g_pUserPaddle;
	delete g_pCompPaddle;
	*/
	delete g_pTile;
	delete g_pUser;

	std::list<CGridTile*>::iterator iter = g_pTiles.begin();
	while (iter != g_pTiles.end())
	{
		delete *iter;
		iter++;
	}
}

void PatteRunGameEngine::Activate()
{
	/*
	g_pBall->Reset();
	g_pCompPaddle->Reset();
	g_pUserPaddle->Reset();
	*/

	int width = Iw2DGetSurfaceWidth();
	int height = Iw2DGetSurfaceHeight();

	CIwFVec2 accuracy = g_pGameHandler->GetAccuracy();
	double widthMeter = g_pGameHandler->GetScaler()->GetMetersPerPixelX() * width;
	double heightMeter = g_pGameHandler->GetScaler()->GetMetersPerPixelY() * height;

	float maxAccuracy = MIN(accuracy.x, accuracy.y);

	double xWidth = widthMeter / maxAccuracy;
	double yWidth = heightMeter / maxAccuracy;

	int iRows = MAX(3, ceil(yWidth));
	int iCols = MAX(3, ceil(xWidth));

	int tileWidth = ceil((double)width / iCols);
	int tileHeight = ceil((double)height / iRows);

	std::list<CGridTile*>::iterator iter = g_pTiles.begin();
	while (iter != g_pTiles.end())
	{
		delete *iter;
		iter++;
	}
	g_pTiles.clear();

	for (int i = 0; i < iCols; ++i)
	{
		for (int j = 0; j < iRows; ++j)
		{
			bool isDark = ((i + j) % 2) == 0;
			CIwRect bounds(i * tileWidth, j * tileHeight, tileWidth, tileHeight);

			CGridTile* pTile = new CGridTile(g_pGameHandler, bounds, isDark);
			g_pTiles.push_back(pTile);
		}
	}

	int lastX = -1;
	int lastY = -1;

	for (int i = 0; i < 20; ++i)
	{
		int xRand, yRand;

		do
		{
			xRand = IwRandMinMax(0, iCols);
			yRand = IwRandMinMax(0, iRows);
		}
		while (xRand == lastX && yRand == lastY);

		g_pCoords[i] = xRand + iCols * yRand;

		lastX = xRand;
		lastY = yRand;
	}

	g_iLevel = 0;
	g_iCounter = -1;
	g_iGameCounter = 0;
	g_iScore = g_iDisplayedScore = 0;

	g_handler.Start(1500, 3, 1000);
}

void PatteRunGameEngine::DeActivate()
{
	g_handler.Stop();
}

uint64 g_lastUpdate = 0;
CGridTile* g_lastActive = NULL;
bool g_bDisplayCorrect = false;

bool PatteRunGameEngine::UpdateLevel()
{
	g_iLevel++;
	if (g_iLevel < 10)
	{
		g_lastUpdate = 0;
		g_iCounter = -1;
		g_iGameCounter = 0;
		return false;
	}
	return true;
}

int PatteRunGameEngine::GetScore()
{
	return g_iScore;
}

bool PatteRunGameEngine::Update(uint64 uiGameTimer, bool* pbVictory)
{
	if (g_iCounter < g_iLevel)
	{
		g_bDisplayCorrect = false;
		if (g_lastUpdate == 0)
		{
			g_lastUpdate = uiGameTimer;
		}
		else if ((uiGameTimer - g_lastUpdate) > 1500)
		{
			g_iCounter++;
			g_lastUpdate = uiGameTimer;
			
			int i = 0;
			std::list<CGridTile*>::iterator iter = g_pTiles.begin();
			while (iter != g_pTiles.end())
			{
				int index = g_pCoords[g_iCounter];
				if (g_pCoords[g_iCounter] == i)
				{
					g_lastActive = *iter;
					(*iter)->SetActive(true);
				}
				else
				{
					(*iter)->SetActive(false);
				}
				i++;
				iter++;
			}
		}
	}
	else
	{
		if ((uiGameTimer - g_lastUpdate) < 1500)
		{
			// delay, then show nothing for a short delay
		}
		else if ((uiGameTimer - g_lastUpdate) < 3000)
		{
			if (g_lastActive)
			{
				g_lastActive->SetActive(false);
			}
		}
		else
		{
			if (g_bDisplayCorrect)
			{
				// we got one right...see if we are done
				if (g_iGameCounter > g_iLevel)
				{
					*pbVictory = true;
					return true;
				}
			}
			g_bDisplayCorrect = false;

			g_pUser->Update(pbVictory);
			int radius = 0;
			CIwVec2* pPos = g_pUser->GetBoundingCircle(&radius);

			int i = 0;
			int activeTile = 0;

			std::list<CGridTile*>::iterator iter = g_pTiles.begin();
			while (iter != g_pTiles.end())
			{
				if ((*iter)->IsContained(pPos))
				{
					activeTile = i;
					g_lastActive = *iter;
					(*iter)->SetActive(true);
				}
				else
				{
					(*iter)->SetActive(false);
				}
				(*iter)->Update(pbVictory);
				iter++;
				i++;
			}

			g_handler.Update();
			float magnitude = g_handler.GetShakeMagnitude();

			if (magnitude != 0)
			{
				if (g_lastActive)
				{
					g_lastActive->SetActive(false);
				}
				int val = g_pCoords[g_iGameCounter];
				// check for accuracy
				if (activeTile == g_pCoords[g_iGameCounter])
				{
					// correct, else game over
					g_iScore += (g_iLevel+1) * pow((g_iGameCounter + 1), 2);

					if (g_iGameCounter < g_iLevel)
					{
						g_bDisplayCorrect = true;
						g_lastUpdate = (uiGameTimer - 1000);
						g_iGameCounter++;
					}
					else
					{
						g_bDisplayCorrect = true;
						g_lastUpdate = (uiGameTimer - 1000);
						g_iGameCounter++;
						//*pbVictory = true;
						//return true;
					}
				}
				else
				{
					*pbVictory = false;
					return true;
				}
			}
		}
	}
	if (g_iDisplayedScore < g_iScore)
	{
		g_iDisplayedScore = MIN(g_iScore, g_iDisplayedScore + 10);
	}
	else if (g_iDisplayedScore > g_iScore)
	{
		g_iDisplayedScore = MAX(g_iScore, g_iDisplayedScore - 10);
	}
	return false;
}

void PatteRunGameEngine::RenderGX()
{
}

void PatteRunGameEngine::Render2D()
{
	std::list<CGridTile*>::iterator iter = g_pTiles.begin();
	while (iter != g_pTiles.end())
	{
		(*iter)->Render();
		iter++;
	}
	Iw2DFinishDrawing();

	if (g_iCounter < g_iLevel)
	{
	}
	else
	{
		g_pUser->Render();

		if (g_bDisplayCorrect)
		{
			IwGxLightingOn();
			IwGxFontSetFont(g_pFontHuge);
			IwGxFontSetCol(0xffffffff);
			IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
			IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
			CIwRect rect(0, 0, Iw2DGetSurfaceWidth(), Iw2DGetSurfaceHeight());
			IwGxFontSetRect(rect);
			strcpy(g_szStatus, "CORRECT!");
			IwGxFontDrawText(g_szStatus);
		}
	}
}

void PatteRunGameEngine::RenderStatus(CIwUIRect& bounds)
{
	IwGxLightingOn();
	IwGxFontSetFont(g_pFont);
	IwGxFontSetCol(0xffffffff);
	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_LEFT);

	CIwRect rect((int16)(bounds.x+5), (int16)bounds.y, (int16)(bounds.w-5), (int16)bounds.h);
	IwGxFontSetRect(rect);
	sprintf(g_szStatus, "%06d", g_iDisplayedScore);
	IwGxFontDrawText(g_szStatus);
	
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_RIGHT);
	IwGxFontSetFont(g_pFontSmall);

	rect.w = 80;
	rect.x = bounds.w - 85;
	IwGxFontSetRect(rect);
	sprintf(g_szStatus, "%02d | %02d", g_iGameCounter, g_iLevel+1);
	IwGxFontDrawText(g_szStatus);

	IwGxLightingOff();

	CIwSVec2 locFly(bounds.w - 85, (bounds.h - g_pTile->GetWidth()) / 2);
	Utils::AlphaRenderImage(g_pTile, locFly, 255);
}


void PatteRunGameEngine::RenderPause()
{
	int width = Iw2DGetSurfaceWidth();
	int height = Iw2DGetSurfaceHeight();

	int iconWidth = 40;
	int iconHeight = 40;

	CIwRect bounds(15, height - 170, width - 30, iconHeight);

	IwGxLightingOn();
	IwGxFontSetFont(g_pFont);
	IwGxFontSetCol(0xffffffff);

	CIwRect rect(0, 0, width, bounds.y);
	IwGxFontSetRect(rect);
	IwGxFontAlignHor alignH = IwGxFontGetAlignmentHor();
	IwGxFontAlignVer alignV = IwGxFontGetAlignmentVer();

	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_CENTRE);
	IwGxFontDrawText("Tap the screen to resume...");
/*
	CIwTexture* pCurrLevel = g_pTextureLevel1;

	switch (g_iLevel)
	{
		case 0 :
		{
			pCurrLevel = g_pTextureLevel1;
		}
		break;
		case 1 :
		{
			pCurrLevel = g_pTextureLevel2;
		}
		break;
		case 2 :
		{
			pCurrLevel = g_pTextureLevel3;
		}
		break;
		case 3 :
		{
			pCurrLevel = g_pTextureLevel4;
		}
		break;
	}


	int totalWidth = g_pTextureLevel->GetWidth() + pCurrLevel->GetWidth() + 10;
	int totalHeight = g_pTextureLevel->GetHeight();

	CIwSVec2 locLevel((width - totalWidth) / 2, rect.h - totalHeight);
	Utils::AlphaRenderImage(g_pTextureLevel, locLevel, 255);

	locLevel.x += (int16)(g_pTextureLevel->GetWidth() + 10);
	Utils::AlphaRenderImage(pCurrLevel, locLevel, 255);

	Iw2DSetAlphaMode(IW_2D_ALPHA_HALF);
	Iw2DFillRect(CIwSVec2(10, height - 170), CIwSVec2(width - 20, 160));
	Iw2DSetAlphaMode(IW_2D_ALPHA_NONE);

	IwGxFontSetAlignmentVer(IW_GX_FONT_ALIGN_MIDDLE);
	IwGxFontSetAlignmentHor(IW_GX_FONT_ALIGN_LEFT);
	IwGxFontSetFont(g_pFontSmall);

	CIwRect fontBounds(15 + iconWidth + 10, height - 170, width - 40 - iconWidth, iconHeight);
	CIwSVec2 loc1(bounds.x + (iconWidth - g_pTextureFirefly->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureFirefly->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureFirefly, loc1, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Capture firelfies to gain points");

	bounds.y += 40;
	fontBounds.y += 40;
	CIwSVec2 loc2(bounds.x + (iconWidth - g_pTextureDragon->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureDragon->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureDragon, loc2, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Dragonflies capture fireflies");

	bounds.y += 40;
	fontBounds.y += 40;
	CIwSVec2 loc3(bounds.x + (iconWidth - g_pTextureMine->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureMine->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureMine, loc3, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Mines take away your points");

	bounds.y += 40;
	fontBounds.y += 40;
	CIwSVec2 loc4(bounds.x + (iconWidth - g_pTextureBolt->GetWidth()) / 2, bounds.y + (iconHeight - g_pTextureBolt->GetHeight()) / 2);
	Utils::AlphaRenderImage(g_pTextureBolt, loc4, 255);
	
	IwGxFontSetRect(fontBounds);
	IwGxFontDrawText("Shake your phone to stun fireflies");
*/
	IwGxLightingOff();
}