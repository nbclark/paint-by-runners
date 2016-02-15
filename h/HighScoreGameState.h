#pragma once
#include "gamestate.h"
#include "IwUI.h"
#include <stdlib.h>
#include <stdio.h>
#include <list>

#define MAX_HIGHSCORES 15

struct HighScore
{
	int Score;
	uint64 Time;
	double DistanceTravelled;
};

class HighScoreGameState :
	public GameState
{
public:
	HighScoreGameState(void);
	~HighScoreGameState(void);

	virtual void PerformUpdate();
	virtual void PerformRender();

	virtual void PerformActivate();
	virtual void PerformDeActivate();

	void AddScore(int newScore, uint64 newTime, double distanceTravelled);

private:
	void ClearScores(bool removeButtons);
	void LoadScores(int newScore, uint64 newTime, double distanceTravelled, bool addScore);
	void SaveScores();
	void OnClickBack(CIwUIElement* Clicked);

	std::list<CIwUIElement*> g_savedMaps;
	int g_iHighScores;
	HighScore g_highScores[MAX_HIGHSCORES];
};

