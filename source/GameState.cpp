#include "GameState.h"

GameState::GameState(void)
{
}

GameState::~GameState(void)
{
}

IGameHandler* GameState::GetGameHandler()
{
	return g_pMapGame;
}

void GameState::SetGameHandler(IGameHandler* pMapGame)
{
	g_pMapGame = pMapGame;
}

void GameState::Update()
{
}

void GameState::Render()
{
}

void GameState::Activate()
{
	//
}

void GameState::DeActivate()
{
}