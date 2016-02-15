#include "SelectGameGameState.h"
#include "Constants.h"
#include "s3e.h"
#include "s3eSocket.h"
#include "MessageBox.h"
#include "tinyxml.h"
#include "Utils.h"

SelectGameGameState::SelectGameGameState(IGameHandler* pGameHandler)
{
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectGameGameState", SelectGameGameState, OnClickStartMulti, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectGameGameState", SelectGameGameState, OnClickStartSingle, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectGameGameState", SelectGameGameState, OnClickBack, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectGameGameState", SelectGameGameState, OnClickUser, CIwUIElement*)
	IW_UI_CREATE_VIEW_SLOT1(this, "SelectGameGameState", SelectGameGameState, OnClickDelete, CIwUIElement*)

	g_pDialogMain = (CIwUIElement*)IwGetResManager()->GetResNamed("selectgame\\panel", "CIwUIElement");
	g_pCreateButton = (CIwUIButton*)g_pDialogMain->GetChildNamed("Button_Create");
	g_pStartButton = (CIwUIButton*)g_pDialogMain->GetChildNamed("Button_Start");

	g_pDialogMain->SetVisible(false);
	IwGetUIView()->AddElement(g_pDialogMain);
	IwGetUIView()->AddElementToLayout(g_pDialogMain);

	g_bWaitingOnGps = true;
	IwGetMultiplayerHandler()->RegisterModeChangedCallback(MultiplayerModeChanged, this);

	g_pUserTexture = new CIwTexture;
	g_pUserTexture->LoadFromFile("images/fireflies/user.png");
	g_pUserTexture->Upload();
	
	g_pUserTextureB = new CIwTexture;
	g_pUserTextureB->LoadFromFile("images/fireflies/userB.png");
	g_pUserTextureB->Upload();

	g_bWaitForStart = false;
	g_bWaitForCreate = false;
	
	g_pBackground = (CIwTexture*)IwGetResManager()->GetResNamed("fullback", "CIwTexture"); 
}

SelectGameGameState::~SelectGameGameState(void)
{
	delete g_pUserTexture;
	delete g_pUserTextureB;
	IW_UI_DESTROY_VIEW_SLOTS(this);
}

void SelectGameGameState::MultiplayerModeChanged(MultiplayerMode mode, void* userData)
{
	if (mode == MPM_IN_GAME)
	{
		// We have started our game
		// Send our state & transition
		SelectGameGameState* pThis = (SelectGameGameState*)userData;
		pThis->g_pMapGame->SetGameState(GPS_GameState_Active, AnimDir_Right);
	}
}

void SelectGameGameState::PerformUpdate()
{
	////IwTrace("FF", ("PerformUpdate"));
	// Get the list of users here...
	
	CIwUIElement* pItems = g_pDialogMain->GetChildNamed("Users");
	CIwUIElement* pButtonTemplate = (CIwUIElement*)IwGetResManager()->GetResNamed("UserTemplate", "CIwUIElement");

	CIwArray<CIwMultiplayerHandler::User*> pUsers = IwGetMultiplayerHandler()->ListUsers();
	CIwArray<CIwMultiplayerHandler::User*> userCopy;
	CIwArray<CIwUIElement*> oldButtons;

	// Delete all buttons unless we have a user for them
	for (uint32 i = 0; i < g_userButtons.size(); ++i)
	{
		bool found = false;

		for (uint32 j = 0; j < pUsers.size(); ++j)
		{
			char* szDevice = pUsers[j]->szDevice;

			CIwUIElement* pTemplate = (CIwUIElement*)pItems->GetChildNamed(pUsers[j]->szDevice, true, true);
			if (pTemplate == g_userButtons[i])
			{
				CIwUIButton* pButtonNew = (CIwUIButton*)pTemplate->GetChildNamed("ButtonBase");
				CIwUILabel* pLabelDistance = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Status", true, false);

				if (pUsers[j]->Accepted)
				{
					if (pUsers[j]->Ready)
					{
						pLabelDistance->SetCaption("Approved. Ready to Begin...");
					}
					else
					{
						pLabelDistance->SetCaption("Approved. Not Ready to Begin...");
					}
				}
				else
				{
					pLabelDistance->SetCaption("Awaiting Approval...");
				}

				found = true;
				break;
			}
		}

		if (!found)
		{
			CIwUIElement* pElement = g_userButtons[i];
			oldButtons.append(pElement);
		}
	}

	for (uint32 i = 0; i < oldButtons.size(); ++i)
	{
		bool result = pItems->GetLayout()->RemoveElement(oldButtons[i]);
		pItems->RemoveChild(oldButtons[i]);
		result = g_userButtons.find_and_remove_fast(oldButtons[i]);
		delete oldButtons[i];
	}

	for (uint32 j = 0; j < pUsers.size(); ++j)
	{
		if (!pItems->GetChildNamed(pUsers[j]->szDevice, false, true))
		{
			userCopy.append(pUsers[j]);
		}
	}

	for (uint32 i = 0; i < userCopy.size(); ++i)
	{
		CIwUIElement* pTemplateNew = pButtonTemplate->Clone();
		CIwUIButton* pButtonNew = (CIwUIButton*)pTemplateNew->GetChildNamed("ButtonBase");
		CIwUIImage* pImage = (CIwUIImage*)pButtonNew->GetChildNamed("ButtonTemplate_Image", true, false);
		CIwUIButton* pImageDelete = (CIwUIButton*)pButtonNew->GetChildNamed("DeleteButton", true, false);
		CIwUILabel* pLabelText = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Text", true, false);
		CIwUILabel* pLabelDistance = (CIwUILabel*)pButtonNew->GetChildNamed("ButtonTemplate_Status", true, false);

		const char* szDevice = userCopy[i]->szDevice;
		const char* szName = userCopy[i]->szName;
		pTemplateNew->SetName(userCopy[i]->szDevice);

		pLabelText->SetCaption(userCopy[i]->szName);
		g_userButtons.append(pTemplateNew);

		CIwMultiplayerHandler::User* pUser = userCopy[i];
		bool isMe = userCopy[i]->IsMe;
		if (userCopy[i]->IsMe)
		{
			pImageDelete->SetVisible(false);
			pImage->SetTexture((CIwTexture*)IwGetResManager()->GetResNamed("user", "CIwTexture"));
		}
		else
		{
			pImage->SetTexture((CIwTexture*)IwGetResManager()->GetResNamed("userB", "CIwTexture"));
		}

		if (userCopy[i]->Accepted)
		{
			if (userCopy[i]->Ready)
			{
				pLabelDistance->SetCaption("Approved. Ready to Begin...");
			}
			else
			{
				pLabelDistance->SetCaption("Approved. Not Ready to Begin...");
			}
		}
		else
		{
			pLabelDistance->SetCaption("Awaiting Approval...");
		}
		//pItems->AddChild(pTemplateNew);
		pItems->GetLayout()->AddElement(pTemplateNew);
	}

	if (IwGetMultiplayerHandler()->ReceivedMapData() && !IwGetMultiplayerHandler()->IsMaster())
	{
		if (!IwGetMultiplayerHandler()->IsReady())
		{
			IwGetMultiplayerHandler()->SetReady();
		}
	}

	if (IwGetMultiplayerHandler()->IsEveryoneReady())
	{
		g_pStartButton->SetEnabled(true);
	}
	else
	{
		g_pStartButton->SetEnabled(false);
	}

	IwGetUIController()->Update();
	IwGetUIView()->Update(1000/20);
}

void SelectGameGameState::PerformRender()
{
	IwGxSetScreenSpaceSlot(-1);
	
	CIwSVec2 backLoc(0,0);
	Utils::AlphaRenderImage(g_pBackground, backLoc, 255);

	IwGxFlush();
	IwGxClear(IW_GX_DEPTH_BUFFER_F);
	IwGxSetScreenSpaceSlot(0);
	IwGetUIView()->Render();
}

void SelectGameGameState::PerformActivate()
{
	g_pStartButton->SetVisible(true);
	g_pStartButton->SetEnabled(true);
	g_pCreateButton->SetEnabled(true);
	
	g_pCreateButton->SetCaption("Play Multi-Player Game");
	g_pStartButton->SetCaption("Play Single-Player Game");

	if (!g_bWaitForStart && !g_bWaitForCreate)
	{
		Region r;
		g_pMapGame->SetBoundingRegion("", &r);
	}
	g_pMapGame->SetActiveUI(g_pDialogMain, NULL);

	if (g_bWaitForStart && strlen(g_pMapGame->GetLoadedRegion()))
	{
		OnClickStartSingle(NULL);
	}
	if (g_bWaitForCreate && strlen(g_pMapGame->GetLoadedRegion()))
	{
		OnClickStartMulti(NULL);
	}

	g_bWaitForStart = false;
	g_bWaitForCreate = false;
}

void SelectGameGameState::PerformDeActivate()
{
	g_pDialogMain->SetVisible(false);
}

void SelectGameGameState::OnClickStartMulti(CIwUIElement* Clicked)
{
	if (strlen(g_pMapGame->GetLoadedRegion()))
	{
		g_pCreateButton->SetEnabled(false);
		g_pStartButton->SetEnabled(false);
		g_pCreateButton->SetCaption("Creating Game...");
		IwGetMultiplayerHandler()->StartMultiplayerGame(CreateStatus, this);
	}
	else
	{
		if (MessageBox::Show("Create or Join?", "Would you like to create or join the multi-player game?", "Create", "Join", NULL, NULL))
		{
			g_bWaitForCreate = true;
			g_pMapGame->SetGameState(GPS_GameState_SelectMap, AnimDir_Right);
		}
		else
		{
			g_pStartButton->SetEnabled(false);
			g_pCreateButton->SetCaption("Searching For Games...");
			IwGetMultiplayerHandler()->JoinMultiplayerGame(JoinStatus, this);
		}
	}
}

void SelectGameGameState::CreateStatus(bool success, const char* szStatus, void* userData)
{
	SelectGameGameState* pThis = (SelectGameGameState*)userData;
	pThis->g_pStartButton->SetEnabled(true);
	pThis->g_pCreateButton->SetEnabled(false);
	pThis->g_pCreateButton->SetCaption(szStatus);
	pThis->g_pStartButton->SetCaption("Start Game");
}

void SelectGameGameState::JoinStatus(bool success, const char* szStatus, void* userData)
{
	SelectGameGameState* pThis = (SelectGameGameState*)userData;
	pThis->g_pStartButton->SetEnabled(false);
	pThis->g_pCreateButton->SetCaption(szStatus);
}

void SelectGameGameState::OnClickStartSingle(CIwUIElement* Clicked)
{
	if (strlen(g_pMapGame->GetLoadedRegion()))
	{
		if (IwGetMultiplayerHandler()->IsMultiplayer())
		{
			IwGetMultiplayerHandler()->StartGame();
		}
		else
		{
			IwGetMultiplayerHandler()->StartSinglePlayerGame();
			g_pMapGame->SetGameState(GPS_GameState_Active, AnimDir_Right);
		}
	}
	else
	{
		g_bWaitForStart = true;
		g_pMapGame->SetGameState(GPS_GameState_SelectMap, AnimDir_Right);
	}
}

void SelectGameGameState::OnClickBack(CIwUIElement* Clicked)
{
	// todo - clean up here
	IwGetMultiplayerHandler()->EndGame();
	g_pMapGame->SetGameState(GPS_GameState_Intro, AnimDir_Right);
}

void SelectGameGameState::OnClickUser(CIwUIElement* Clicked)
{
	CIwUIElement* p1 = Clicked->GetParent();
	CIwUIElement* p2 = Clicked->GetParent()->GetParent();
	CIwUIElement* p3 = Clicked->GetParent()->GetParent()->GetParent();
	CIwUIElement* pParent = Clicked->GetParent()->GetParent();

	// accept the user
	CIwArray<CIwMultiplayerHandler::User*> pUsers = IwGetMultiplayerHandler()->ListUsers();

	if (IwGetMultiplayerHandler()->IsMaster())
	{
		for (uint32 j = 0; j < pUsers.size(); ++j)
		{
			if (pParent->GetChildNamed(pUsers[j]->szDevice, false, true) == p1)
			{
				if (!pUsers[j]->Accepted)
				{
					if (MessageBox::Show("Approve/Reject Player", "Would you like to approve or reject this player?", "Approve", "Reject", NULL, NULL))
					{
						IwGetMultiplayerHandler()->AcceptUser(pUsers[j]);
					}
					else
					{
						IwGetMultiplayerHandler()->RejectUser(pUsers[j]);
					}
					break;
				}
			}
		}
	}
}

void SelectGameGameState::OnClickDelete(CIwUIElement* Clicked)
{
	CIwUIElement* p1 = Clicked->GetParent();
	CIwUIElement* p2 = Clicked->GetParent()->GetParent();
	CIwUIElement* p3 = Clicked->GetParent()->GetParent()->GetParent();
	CIwUIElement* pParent = Clicked->GetParent()->GetParent()->GetParent();

	// accept the user
	CIwArray<CIwMultiplayerHandler::User*> pUsers = IwGetMultiplayerHandler()->ListUsers();

	if (IwGetMultiplayerHandler()->IsMaster())
	{
		for (uint32 j = 0; j < pUsers.size(); ++j)
		{
			if (pParent->GetChildNamed(pUsers[j]->szDevice, false, true) == p2)
			{
				if (!pUsers[j]->IsMe || true)
				{
					if (MessageBox::Show("Remove User", "Are you sure you want to remove this user from the game?", "Yes", "No", NULL, NULL))
					{
						IwGetMultiplayerHandler()->RejectUser(pUsers[j]);
						break;
					}
				}
			}
		}
	}
}