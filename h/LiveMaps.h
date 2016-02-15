#ifndef __LiveMaps
#define __LiveMaps

#include "s3eLocation.h"
#include "Iw2D.h"
#include "IwTexture.h"
#include "IwHTTP.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <list>

#define MilesPerMeter 0.000621371192237334f
#define _tileWidth 256
#define _tileHeight 256

struct GPSRectangle
{
	s3eLocation topLeft;
	s3eLocation topRight;
	s3eLocation bottomLeft;
	s3eLocation bottomRight;
};

struct MapTile
{
	char szImageUrl[200];
	char szImagePngUrl[200];
	CIwTexture* pTexture;
	CIwSVec2 location;
	bool bInProgress;
	bool bAttemptedJpg;
	int row;
	int col;
	int retryCount;
};

class LiveMaps
{
public :
	static int MaxZoom;
	static int MaxPerfZoom;
	static double EarthRadius;
	static double EarthCircum;
	static double EarthHalfCircum;

public:
	static void CalculateTriangleAngles(double a, double b, double c, double* angleA, double* angleB, double* angleC);
	static double CalculateTriangleAngles(s3eLocation& a, s3eLocation& b, s3eLocation& c);
	static void ScaleAnglesTo90(double* angleA, double* angleB, double angleIncrement);

	static double DegToRad(double d);
	static double RadToDeg(double d);
	static int LongitudeToXAtZoom(double lon, int zoom);
	static int LatitudeToYAtZoom(double lat, int zoom);
	static double MetersPerPixel(int zoom);
	static double YToLatitudeAtZoom(int y, int zoom);
	static double XToLongitudeAtZoom(int x, int zoom);
	static void TileToQuadKey(char* szQuad, int tx, int ty, int zoom);
	static double CalculateSlopeAngleBetweenPoints(double latitudeA, double longitudeA, double latitudeB, double longitudeB);
	static double CalculateSlopeAngleBetweenPoints(s3eLocation* pLocationA, s3eLocation* pLocationB);
	static double CalculateDistance(s3eLocation& pLocationA, s3eLocation& pLocationB);
	static s3eLocation CalculateCenter(std::list<s3eLocation>& pLocations)
	{
		s3eLocation location;

		if (pLocations.size() > 0)
		{
			double maxLat = -1000, minLat = 1000, maxLon = -1000, minLon = 1000;

			std::list<s3eLocation>::iterator iter = pLocations.begin();
			while (iter != pLocations.end())
			{
				s3eLocation loc = *iter;
				maxLat = MAX(maxLat, loc.m_Latitude);
				minLat = MIN(minLat, loc.m_Latitude);
				
				maxLon = MAX(maxLon, loc.m_Longitude);
				minLon = MIN(minLon, loc.m_Longitude);

				iter++;
			}
			location.m_Latitude = (maxLat + minLat) / 2;
			location.m_Longitude = (maxLon + minLon) / 2;
		}
		return location;
	}
	static double CalculateDistance(s3eLocation* pLocationA, s3eLocation* pLocationB);
	static double CalculateDistance(double latitudeA, double longitudeA, double latitudeB, double longitudeB, double middleLatitude);
	static void CalculateLatLongInDirection(s3eLocation* pLocation, double distance, double angle, s3eLocation* pReturn);
	static int EstimateZoom(GPSRectangle* pCorners, int width, int height, double maxZoom);
	static void GetRectangleImageUrls(std::list<MapTile*>* pImages, int* pRows, int* pCols, GPSRectangle* pCorners, int width, int height, double rotationAngle, double maxZoom, GPSRectangle* pScreenCorners, double* pLatPerPixel, double* pLonPerPixel);

	static void GetLocationImages(std::list<MapTile*>* pImages, int* pRows, int* pCols, s3eLocation* pLocation, int width, int height, GPSRectangle* pCorner, GPSRectangle* pScreenCorners, double maxZoom, double* latPerPixel, double* lonPerPixel);
};

#endif