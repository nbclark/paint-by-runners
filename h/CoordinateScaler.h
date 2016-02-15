#pragma once
#include "Iw2D.h"
#include "LiveMaps.h"

#ifndef MAX4
	#define	MAX4(a, b, c, d)	MAX(MAX(a,b), MAX(c,d))
#endif

#ifndef MIN4
	#define	MIN4(a, b, c, d)	MIN(MIN(a,b), MIN(c,d))
#endif

class CoordinateScaler
{
public:
	CoordinateScaler(int width, int height, GPSRectangle* pCorners, double angle, bool stretchToFit);
	~CoordinateScaler(void);

	GPSRectangle* GetCorners();

	void SetSize(int width, int height);
	void SetCorners(GPSRectangle* pCorners, int zoom, bool stretchToFit);
	void SetBaseScaler(CoordinateScaler* pBaseScaler);

	GPSRectangle GetScaledCorners();
	CIwMat GetDrawRotationMatrix(double progress);

	double GetRotationAngle();
	double GetScaleX();
	double GetScaleY();

	double GetZoom()
	{
		if (!g_curZoom)
		{
			s3eDeviceYield(0);
		}
		return g_curZoom;
	}

	double GetCenterLatitude();
	double GetCenterLongitude();

	double GetMetersPerPixelX();
	double GetMetersPerPixelY();
	CIwFVec2 GetSpeedInPixels(CIwFVec2& metersPerSecond);

	bool GetStretchToFit();
	int GetWidth();
	int GetHeight();

	void LocationToPosition(s3eLocation& pLocation, CIwSVec2* pPosition);
	void PositionToLocation(CIwSVec2& pPosition, s3eLocation* pLocation);

private:
	CIwFMat GetInverseRotationMatrix(double progress);
	CIwMat GetRotationMatrix(double progress);
	inline void InitScale();

	int g_width, g_height, g_scaledWidth, g_scaledHeight, g_minX, g_minY, g_maxX, g_maxY;
	bool g_bStretchToFit, g_inverted, g_scaleInit;
	double g_angle, g_lastProgress;
	float g_curZoom;
	GPSRectangle g_pCorners;
	GPSRectangle g_pScaledCorners;
	s3eLocation g_centerLocation;
	CIwMat g_rotationMatrix, g_drawRotationMatrix;
	CIwFMat g_invRotationMatrix;
	CoordinateScaler* g_pBaseScaler;
	
    double g_xScale, g_yScale, g_p1To2Distance, g_rotationAngle, g_metersPerPixelX, g_metersPerPixelY;
};

