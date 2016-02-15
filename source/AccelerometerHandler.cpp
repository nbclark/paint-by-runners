#include "s3e.h"
#include "AccelerometerHandler.h"

void AccelerometerHandler::Start(float minShakeMagnitude, int shakeCount, uint64 maxShakeDuration)
{
	s3eAccelerometerStart();
	g_minShakeMagnitude = minShakeMagnitude;
	g_shakeCount = shakeCount;
	g_maxShakeDuration = maxShakeDuration;
	
	g_prevX = s3eAccelerometerGetX();
	g_prevY = s3eAccelerometerGetY();
	g_prevZ = s3eAccelerometerGetZ();

	g_prevDirX = 0;
	g_prevDirY = 0;
	g_prevDirZ = 0;

	g_bIsInit = true;
}

void AccelerometerHandler::Reset()
{
	std::list<ShakeEvent*>::iterator shakeIter = g_shakeTimes.begin();
	
	while (shakeIter != g_shakeTimes.end())
	{
		delete *shakeIter;
		shakeIter++;
	}

	g_shakeTimes.clear();
}

void AccelerometerHandler::Stop()
{
	g_bIsInit = false;
	s3eAccelerometerStop();
	
	Reset();
}

void AccelerometerHandler::Update()
{
	if (g_bIsInit)
	{
		uint64 timerMs = s3eTimerGetMs();

		int32 x = s3eAccelerometerGetX();
		int32 y = s3eAccelerometerGetY();
		int32 z = s3eAccelerometerGetZ();

		int32 diffX = (x - g_prevX);
		int32 diffY = (y - g_prevY);
		int32 diffZ = (z - g_prevZ);

		// Get the change in magnitude
		float magnitude = (float)sqrt((diffX*diffX) + (diffY*diffY) + (diffZ*diffZ));

		// Clear out old events
		std::list<ShakeEvent*>::iterator shakeIter = g_shakeTimes.begin();
		std::list<ShakeEvent*> oldEvents;

		while (shakeIter != g_shakeTimes.end())
		{
			if ((timerMs-(*shakeIter)->timerMs) > g_maxShakeDuration)
			{
				oldEvents.push_back(*shakeIter);
			}
			shakeIter++;
		}

		shakeIter = oldEvents.begin();

		while (shakeIter != oldEvents.end())
		{
			g_shakeTimes.remove(*shakeIter);
			delete *shakeIter;
			shakeIter++;
		}

		int32 dirX = SIGN(x);
		int32 dirY = SIGN(y);
		int32 dirZ = SIGN(z);

		bool hasDirChange = false;
		if ( (dirX != g_prevDirX) || (dirY != g_prevDirY) || (dirZ != g_prevDirZ) )
		{
			hasDirChange = true;

			g_prevDirX = dirX;
			g_prevDirY = dirY;
			g_prevDirZ = dirZ;
		}

		if (hasDirChange && magnitude > g_minShakeMagnitude)
		{
			ShakeEvent* pEvent = new ShakeEvent;
			pEvent->timerMs = timerMs;
			pEvent->shakeMag = magnitude;

			g_shakeTimes.push_back(pEvent);
		}

		g_prevX = x;
		g_prevY = y;
		g_prevZ = z;
	}
}

float AccelerometerHandler::GetShakeMagnitude()
{
	if (g_bIsInit && g_shakeTimes.size() >= g_shakeCount)
	{
		int count = g_shakeTimes.size();
		float maxShakeMagnitude = 0;

		std::list<ShakeEvent*>::iterator shakeIter = g_shakeTimes.begin();
		while (shakeIter != g_shakeTimes.end())
		{
			ShakeEvent* pEvent = *shakeIter;
			maxShakeMagnitude = MAX(pEvent->shakeMag, maxShakeMagnitude);
			shakeIter++;
		}

		// Return a scale of 0-10 most likely
		return (maxShakeMagnitude / g_minShakeMagnitude);
	}
	return 0;
}
