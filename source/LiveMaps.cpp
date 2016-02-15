#include "LiveMaps.h"

int LiveMaps::MaxZoom = 19;
int LiveMaps::MaxPerfZoom = 18;
double LiveMaps::EarthRadius = 6378137;
double LiveMaps::EarthCircum = EarthRadius * 2.0 * PI;
double LiveMaps::EarthHalfCircum = EarthCircum / 2;

double LiveMaps::DegToRad(double d)
{
    return d * PI / 180;
}
double LiveMaps::RadToDeg(double d)
{
    return d * 180 / PI;
}

int LiveMaps::LongitudeToXAtZoom(double lon, int zoom)
{
    double arc = EarthCircum / ((1 << zoom) * _tileWidth);
    double metersX = EarthRadius * DegToRad(lon);
    return (int)ceil((metersX + EarthHalfCircum) / arc);
} 

double LiveMaps::MetersPerPixel(int zoom)
{
    double arc = EarthCircum / ((1 << zoom) * 256);
    return arc;
}


int LiveMaps::LatitudeToYAtZoom(double lat, int zoom)
{
    double arc = EarthCircum / ((1 << zoom) * _tileHeight);
    double sinLat = sin(DegToRad(lat));
    double metersY = EarthRadius / 2 * log((1.0 + sinLat) / (1.0 - sinLat));
    return (int)ceil((EarthHalfCircum - metersY) / arc);
}

double LiveMaps::YToLatitudeAtZoom(int y, int zoom)
{
    double arc = EarthCircum / ((1 << zoom) * _tileHeight);
    double metersY = EarthHalfCircum - (y * arc);
    double a = exp(metersY * 2 / EarthRadius);
    double result = RadToDeg(asin((a - 1) / (a + 1)));
    return result;
}

double LiveMaps::XToLongitudeAtZoom(int x, int zoom)
{
    double arc = EarthCircum / ((1 << zoom) * _tileWidth);
    double metersX = (x * arc) - EarthHalfCircum;
    double result = RadToDeg(metersX / EarthRadius);
    return result;
}

void LiveMaps::TileToQuadKey(char* szQuad, int tx, int ty, int zoom)
{
	szQuad[0] = 0;
    for (int i = zoom; i > 0; i--)
    {
        int mask = 1 << (i - 1);
        int cell = 0;

        if ((tx & mask) != 0)
        {
            cell = cell + 1;
        }
        if ((ty & mask) != 0)
        {
            cell = cell + 2;
        }
        sprintf(szQuad, "%s%i", szQuad, cell);
    }
}

double LiveMaps::CalculateSlopeAngleBetweenPoints(double latitudeA, double longitudeA, double latitudeB, double longitudeB)
{
    double distanceBetweenPoints = CalculateDistance(latitudeA, longitudeA, latitudeB, longitudeB, 0);
    double longComponent = (CalculateDistance(latitudeA, longitudeA, latitudeA, longitudeB, 0) + CalculateDistance(latitudeB, longitudeA, latitudeB, longitudeB, 0)) / 2;

    bool latGreater = latitudeB > latitudeA;
    bool lonGreater = longitudeB > longitudeA;

    double angle = acos(longComponent / distanceBetweenPoints);

    if (latGreater && lonGreater)
    {
        // do nothing -- angle = angle
    }
    else if (latGreater && !lonGreater)
    {
        angle = PI - angle;
    }
    else if (!latGreater && lonGreater)
    {
        angle = 0 - angle;
    }
    else
    {
        angle = (PI + angle) - (2 * PI);
    }

    return angle;
}

double LiveMaps::CalculateSlopeAngleBetweenPoints(s3eLocation* pLocationA, s3eLocation* pLocationB)
{
    return CalculateSlopeAngleBetweenPoints(pLocationA->m_Latitude, pLocationA->m_Longitude, pLocationB->m_Latitude, pLocationB->m_Longitude);
}

double LiveMaps::CalculateDistance(s3eLocation* pLocationA, s3eLocation* pLocationB)
{
	return CalculateDistance(pLocationA->m_Latitude, pLocationA->m_Longitude, pLocationB->m_Latitude, pLocationB->m_Longitude, 0);
}

double LiveMaps::CalculateDistance(s3eLocation& pLocationA, s3eLocation& pLocationB)
{
	return CalculateDistance(pLocationA.m_Latitude, pLocationA.m_Longitude, pLocationB.m_Latitude, pLocationB.m_Longitude, 0);
}

// Since we are doing small calculations, we need to assume all calculations are done along the same latitude
double LiveMaps::CalculateDistance(double latitudeA, double longitudeA, double latitudeB, double longitudeB, double middleLatitude)
{
    latitudeA = latitudeA - middleLatitude;
    latitudeB = latitudeB - middleLatitude;

    ////System.Diagnostics.Debug.WriteLine(String.Format("latA = {0}, latB = {1}, lonA = {2}, lonB = {3}", latitudeA, latitudeB, longitudeA, longitudeB));

    double dLat1InRad = latitudeA * (PI / 180.0);
    double dLong1InRad = longitudeA * (PI / 180.0);
    double dLat2InRad = latitudeB * (PI / 180.0);
    double dLong2InRad = longitudeB * (PI / 180.0);

    double dLongitude = dLong2InRad - dLong1InRad;
    double dLatitude = dLat2InRad - dLat1InRad;

    // Intermediate result a.
    double a = pow(sin(dLatitude / 2.0), 2.0) +
               cos(dLat1InRad) * cos(dLat2InRad) *
               pow(sin(dLongitude / 2.0), 2.0);

    // Intermediate result c (great circle distance in Radians).
    // arcsin works just as well
    double c1 = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    double c = 2.0 * asin(sqrt(a));

    // Distance.
    // const Double kEarthRadiusMiles = 3956.0;
    double earthRadiusMeters = 6376.5 * 1000;
    double pointDistance = earthRadiusMeters * c;

    return pointDistance;
}

void LiveMaps::CalculateLatLongInDirection(s3eLocation* pLocation, double distance, double angle, s3eLocation* pNewLocation)
{
    // Get the distance between longitude lines at given latitude
    double longLineDistance = CalculateDistance(pLocation->m_Latitude, pLocation->m_Longitude, pLocation->m_Latitude, pLocation->m_Longitude + 1, 0);
    double latLineDistance = CalculateDistance(pLocation->m_Latitude, pLocation->m_Longitude, pLocation->m_Latitude + 1, pLocation->m_Longitude, 0);

    double longChange = distance * cos(angle);
    double latChange = distance * sin(angle);

    ////System.Diagnostics.Debug.WriteLine(String.Format("latChange = {0}, lonChange = {1}", latChange, longChange));

    double longChangeInDegrees = longChange / longLineDistance;
    double latChangeInDegrees = latChange / latLineDistance;

	pNewLocation->m_Latitude = pLocation->m_Latitude + latChangeInDegrees;
	pNewLocation->m_Longitude = pLocation->m_Longitude + longChangeInDegrees;
}


int LiveMaps::EstimateZoom(GPSRectangle* pCorners, int width, int height, double maxZoom)
{
    double maxLat = -DBL_MAX, minLat = DBL_MAX;
    double maxLon = -DBL_MAX, minLon = DBL_MAX;

	s3eLocation pLocations[4];
	pLocations[0] = pCorners->bottomLeft;
	pLocations[1] = pCorners->bottomRight;
	pLocations[2] = pCorners->topLeft;
	pLocations[3] = pCorners->topRight;

    for (int i = 0; i < 4; ++i)
    {
		s3eLocation* pLocation = &pLocations[i];

        if (pLocation->m_Longitude < minLon)
        {
            minLon = pLocation->m_Longitude;
        }
        if (pLocation->m_Longitude > maxLon)
        {
            maxLon = pLocation->m_Longitude;
        }
        if (pLocation->m_Latitude < minLat)
        {
            minLat = pLocation->m_Latitude;
        }
        if (pLocation->m_Latitude > maxLat)
        {
            maxLat = pLocation->m_Latitude;
        }
    }

    double arc = EarthCircum / _tileHeight;
    double sinLat1 = sin(DegToRad(maxLat));
    double metersY1 = EarthRadius / 2 * log((1.0 + sinLat1) / (1.0 - sinLat1));
    double sinLat2 = sin(DegToRad(minLat));
    double metersY2 = EarthRadius / 2 * log((1.0 + sinLat2) / (1.0 - sinLat2));
    double yDifference = ((EarthHalfCircum - metersY2) / arc) - ((EarthHalfCircum - metersY1) / arc);

    double metersX1 = EarthRadius * DegToRad(maxLon);
    double metersX2 = EarthRadius * DegToRad(minLon);
    double xDifference = ((EarthHalfCircum - metersX2) / arc) - ((EarthHalfCircum - metersX1) / arc);

	int yDist1 = (int)CalculateDistance(minLat, maxLon, maxLat, maxLon, 0);
	int yDist2 = (int)CalculateDistance(minLat, minLon, maxLat, minLon, 0);

	int xDist1 = (int)CalculateDistance(minLat, minLon, minLat, maxLon, 0);
	int xDist2 = (int)CalculateDistance(maxLat, minLon, maxLat, maxLon, 0);

    double zoomY = ceil(ABS(log(yDifference / height / _tileHeight))) + 1;
    double zoomX = ceil(ABS(log(xDifference / width / _tileWidth))) + 1;

	while (true)
	{
		int xDiff = (int)ABS(LongitudeToXAtZoom(maxLon, (int)maxZoom) - LongitudeToXAtZoom(minLon, (int)maxZoom));
		int yDiff = (int)ABS(LatitudeToYAtZoom(maxLat, (int)maxZoom) - LatitudeToYAtZoom(minLat, (int)maxZoom));

		if ((xDiff-width) <= 0 && (yDiff-height) <= 0)
		{
			break;
		}
		maxZoom--;
	}
	return (int)maxZoom;

    return (int)MIN(maxZoom, MAX(zoomX, zoomY));
}

void LiveMaps::GetRectangleImageUrls(std::list<MapTile*>* pImages, int* pRows, int* pCols, GPSRectangle* pCorners, int width, int height, double rotationAngle, double maxZoom, GPSRectangle* pScreenCorners, double* pLatPerPixel, double* pLonPerPixel)
{
    int maxSide = (int)MAX(width, height);

    int zoom = EstimateZoom(pCorners, width, height, maxZoom);
	zoom = (int)maxZoom;
    int maxTx = 0, maxTy = 0, minTx = INT_MAX, minTy = INT_MAX;

	s3eLocation pLocations[4];
	pLocations[0] = pCorners->topLeft;
	pLocations[1] = pCorners->topRight;
	pLocations[2] = pCorners->bottomLeft;
	pLocations[3] = pCorners->bottomRight;

    for (int i = 0; i < 4; ++i)
    {
        int tx = LongitudeToXAtZoom(pLocations[i].m_Longitude, zoom) / _tileWidth;
        int ty = LatitudeToYAtZoom(pLocations[i].m_Latitude, zoom) / _tileHeight;

        if (tx < minTx)
        {
            minTx = tx;
        }
        if (tx > maxTx)
        {
            maxTx = tx;
        }
        if (ty < minTy)
        {
            minTy = ty;
        }
        if (ty > maxTy)
        {
            maxTy = ty;
        }
    }

    double minLat = YToLatitudeAtZoom(minTy * _tileHeight, zoom);
    double maxLat = YToLatitudeAtZoom((maxTy + 1) * _tileHeight, zoom);
    double minLon = XToLongitudeAtZoom(minTx * _tileWidth, zoom);
    double maxLon = XToLongitudeAtZoom((maxTx + 1) * _tileWidth, zoom);

	*pCols = (maxTx-minTx+1);
	*pRows = (maxTy-minTy+1);

    width = _tileWidth * (maxTx-minTx+1);
    height = _tileHeight * (maxTy-minTy+1);

    double latPerPixel = (maxLat - minLat) / height;
    double lonPerPixel = (maxLon - minLon) / width;

	*pLatPerPixel = latPerPixel;
	*pLonPerPixel = lonPerPixel;

    double minX = (double)width,
		minY = (double)height,
		maxX = 0,
		maxY = 0;

    int index = 0;
    for (int i = 0; i < 4; ++i)
    {
        double x = 0, y = 0;

        x = ((pLocations[i].m_Longitude - minLon) / lonPerPixel);
        y = ((pLocations[i].m_Latitude - minLat) / latPerPixel);

        //pPointCorners[index]->x = (float)x;
        //pPointCorners[index]->y = (float)y;

		index++;

        if (x < minX)
        {
            minX = x;
        }
        if (x > maxX)
        {
            maxX = x;
        }
        if (y < minY)
        {
            minY = y;
        }
        if (y > maxY)
        {
            maxY = y;
        }
    }

	int top = (int)((pLocations[0].m_Latitude - minLat) / latPerPixel);
	int left = (int)((pLocations[0].m_Longitude - minLon) / lonPerPixel);

    for (int i = 0; i < 4; ++i)
    {
        //pPointCorners[i]->x = pPointCorners[i]->x;// -(int)minX;
        //pPointCorners[i]->y = pPointCorners[i]->y;// -(int)minY;
    }

	char szQuad[20];

	int count = 0;

	// Go 1 before and after the min/max TX and TY so that we pre-load tiles
    for (int i = minTx; i <= maxTx; ++i)
    {
        for (int j = minTy; j <= maxTy; ++j)
        {
			float x = (float)(_tileWidth*(i-minTx)) - (left);
			float y = (float)(_tileHeight*(j-minTy)) - (top);

            int server = 0;

			TileToQuadKey(szQuad, i, j, zoom);
			
			MapTile* pTile = new MapTile;
			pTile->row = (i-minTx);
			pTile->col = (j-minTy);
			pTile->pTexture = NULL;
			pTile->bInProgress = false;
			pTile->location.x = (int)x;
			pTile->location.y = (int)y;
			pTile->retryCount = 0;
			pImages->push_back(pTile);
			sprintf(pTile->szImageUrl, "http://r%i.ortho.tiles.virtualearth.net/tiles/%s%s.%s?g=22", server, "h", szQuad, "jpg");
			sprintf(pTile->szImagePngUrl, "http://r%i.ortho.tiles.virtualearth.net/tiles/%s%s.%s?g=22", server, "r", szQuad, "png");

			count++;
        }
    }

    return;
}

void LiveMaps::GetLocationImages(std::list<MapTile*>* pImages, int* pRows, int* pCols, s3eLocation* pLocation, int width, int height, GPSRectangle* pCorner, GPSRectangle* pScreenCorners, double maxZoom, double* latPerPixel, double* lonPerPixel)
{
    double slope = atan((double)height / width);

	int x = LongitudeToXAtZoom(pLocation->m_Longitude, (int)maxZoom);
	int y = LatitudeToYAtZoom(pLocation->m_Latitude, (int)maxZoom);

	double longitude1 = XToLongitudeAtZoom(x, (int)maxZoom);
	double latitude1 = YToLatitudeAtZoom(y, (int)maxZoom);

	double longitude = XToLongitudeAtZoom(x+width, (int)maxZoom);
	double latitude = YToLatitudeAtZoom(y+height, (int)maxZoom);

	double xDistance = CalculateDistance(pLocation->m_Latitude, pLocation->m_Longitude, pLocation->m_Latitude, longitude, 0);
	double yDistance = CalculateDistance(pLocation->m_Latitude, pLocation->m_Longitude, latitude, pLocation->m_Longitude, 0);

	double xMet = xDistance / width;
	double yMet = yDistance / height;

	double metersPerPixel = (ABS(xMet)+ABS(yMet)) / 2;

    double distance = sqrt((pow(height, 2) + pow(width, 2)) / 4) * metersPerPixel;

	// Find the corners of the screen
    CalculateLatLongInDirection(pLocation, distance, PI - slope, &pCorner->topLeft);
    CalculateLatLongInDirection(pLocation, distance, slope, &pCorner->topRight);
    CalculateLatLongInDirection(pLocation, distance, PI + slope, &pCorner->bottomLeft);
    CalculateLatLongInDirection(pLocation, distance, 0 - slope, &pCorner->bottomRight);

    GetRectangleImageUrls(pImages, pRows, pCols, pCorner, width, height, 0, maxZoom, pScreenCorners, latPerPixel, lonPerPixel);
}

double LiveMaps::CalculateTriangleAngles(s3eLocation& a, s3eLocation& b, s3eLocation& c)
{
    double tempAngleA, tempAngleB, middleAngle;
    double sideAB = CalculateDistance(a, b);
    double sideBC = CalculateDistance(b, c);
    double sideCA = CalculateDistance(c, a);

    CalculateTriangleAngles(sideAB, sideBC, sideCA, &tempAngleA, &tempAngleB, &middleAngle);

    return middleAngle;
}

void LiveMaps::CalculateTriangleAngles(double a, double b, double c, double* angleA, double* angleB, double* angleC)
{
    double cosValue = (pow(b, 2) + pow(c, 2) - pow(a, 2)) / (2 * b * c);

    if (cosValue > 1 || cosValue < -1)
    {
        *angleA = 0;
		*angleB = 0;
		*angleC = 0;
    }
    else
    {
        *angleA = acos(cosValue);
        *angleB = asin(b * sin(*angleA) / a);
        *angleC = PI - (*angleA + *angleB);
    }
}
void LiveMaps::ScaleAnglesTo90(double* angleA, double* angleB, double angleIncrement)
{
    *angleA = *angleA - angleIncrement;
    *angleB = *angleB + angleIncrement;

    angleIncrement = (*angleA - *angleB - PI / 2) / 2;

    *angleA = *angleA - angleIncrement;
    *angleB = *angleB + angleIncrement;
}