#ifndef _AccelerometerHandler
#define _AccelerometerHandler
#include "IwGx.h"
#include <list>

#ifndef SIGN
	#define	SIGN(a)			(((a) > 0) ? (a) : -(a))
#endif

struct ShakeEvent
{
	uint64 timerMs;
	float shakeMag;
};

class AccelerometerHandler
{
public:
	AccelerometerHandler()
	{
		g_bIsInit = false;
	}
	void Start(float minShakeMagnitude, int shakeCount, uint64 maxShakeDuration);
	void Stop();
	void Update();
	void Reset();

	// Returns 0 - N, where the value is the magnitude of the shake over the threshold
	float GetShakeMagnitude();

private:
	std::list<ShakeEvent*> g_shakeTimes;
	
	float g_minShakeMagnitude;
	uint16 g_shakeCount;
	uint64 g_maxShakeDuration;
	int g_prevX, g_prevY, g_prevZ;
	bool g_bIsInit;

	int g_prevDirX, g_prevDirY, g_prevDirZ;
};

#endif

