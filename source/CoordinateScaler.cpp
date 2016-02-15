#include "CoordinateScaler.h"

CoordinateScaler::CoordinateScaler(int width, int height, GPSRectangle* pCorners, double angle, bool stretchToFit)
{
	g_rotationAngle = 0;
	g_metersPerPixelX = LiveMaps::MetersPerPixel(LiveMaps::MaxZoom);
	g_metersPerPixelY = LiveMaps::MetersPerPixel(LiveMaps::MaxZoom);
	g_width = width;
	g_height = height;
	g_minX = g_minY = INT_MIN;
	g_maxX = g_maxY = INT_MAX;

	g_xScale = g_yScale = 1;
	g_curZoom = 19;
	g_lastProgress = -1;

	g_scaleInit = false;

	if (pCorners)
	{
		memcpy(&g_pCorners, pCorners, sizeof(GPSRectangle));
	}
	g_angle = angle;
	g_bStretchToFit = stretchToFit;
	g_pBaseScaler = NULL;

	g_drawRotationMatrix.SetIdentity();
	g_rotationMatrix.SetIdentity();
	g_invRotationMatrix.SetIdentity();
}

CoordinateScaler::~CoordinateScaler(void)
{
}

void CoordinateScaler::SetSize(int width, int height)
{
	g_width = width;
	g_height = height;
}

void CoordinateScaler::SetBaseScaler(CoordinateScaler* pBaseScaler)
{
	g_pBaseScaler = pBaseScaler;
}

GPSRectangle* CoordinateScaler::GetCorners()
{
	return &g_pCorners;
}

void CoordinateScaler::SetCorners(GPSRectangle* pCorners, int zoom, bool stretchToFit)
{
		if (!g_curZoom)
		{
			s3eDeviceYield(0);
		}
	g_curZoom = (float)zoom;
		if (!g_curZoom)
		{
			s3eDeviceYield(0);
		}
	g_bStretchToFit = stretchToFit;
	g_pBaseScaler = NULL;
	g_lastProgress = -1;
	g_scaleInit = false;

	g_metersPerPixelX = LiveMaps::MetersPerPixel(zoom);
	g_metersPerPixelY = LiveMaps::MetersPerPixel(zoom);

	if (!stretchToFit)
	{
		memcpy(&g_pCorners, pCorners, sizeof(GPSRectangle));
		g_rotationAngle = 0;
		g_xScale = 1;
		g_yScale = 1;
		g_scaleInit = true;
	}
	else
	{
		memcpy(&g_pScaledCorners, pCorners, sizeof(GPSRectangle));

		double canvasWidth = MAX(LiveMaps::CalculateDistance(pCorners->topLeft, pCorners->topRight), LiveMaps::CalculateDistance(pCorners->bottomLeft, pCorners->bottomRight));
		double canvasHeight = MAX(LiveMaps::CalculateDistance(pCorners->topLeft, pCorners->bottomLeft), LiveMaps::CalculateDistance(pCorners->topRight, pCorners->bottomRight));

		int x = LiveMaps::LongitudeToXAtZoom(pCorners->topLeft.m_Longitude, g_curZoom);
		int y = LiveMaps::LatitudeToYAtZoom(pCorners->topLeft.m_Latitude, g_curZoom);
		
		double longitude1 = LiveMaps::XToLongitudeAtZoom(x + 100, g_curZoom);
		double latitude1 = LiveMaps::YToLatitudeAtZoom(y + 100, g_curZoom);

		double latPerPixel = ABS(pCorners->topLeft.m_Latitude-latitude1);
		double lonPerPixel = ABS(pCorners->topLeft.m_Longitude-longitude1);

		double xMetersPerPixel = LiveMaps::CalculateDistance(pCorners->topLeft.m_Latitude, pCorners->topLeft.m_Longitude, pCorners->topLeft.m_Latitude, longitude1, 0) / 100;
		double yMetersPerPixel = LiveMaps::CalculateDistance(pCorners->topLeft.m_Latitude, pCorners->topLeft.m_Longitude, latitude1, pCorners->topLeft.m_Longitude, 0) / 100;

		double metPerPixel = (ABS(xMetersPerPixel)+ABS(yMetersPerPixel)) / 2;

		canvasWidth = canvasWidth / metPerPixel;
		canvasHeight = canvasHeight / metPerPixel;

		// TODO: we should really scale proportionally
		g_xScale = canvasWidth / g_width;
		g_yScale = canvasHeight / g_height;

		if ((canvasWidth / g_width) + (canvasHeight / g_height) > (canvasHeight / g_width) + (canvasWidth / g_height))
		{
			//memcpy(&g_pCorners.topLeft, &pCorners->topLeft, sizeof(s3eLocation));
			//memcpy(&g_pCorners.topRight, &pCorners->topRight, sizeof(s3eLocation));

			g_xScale = canvasHeight / g_width;
			g_yScale = canvasWidth / g_height;

			g_inverted = true;

			g_rotationAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(&pCorners->bottomLeft, &pCorners->topLeft);
		}
		else
		{
			//memcpy(&g_pCorners.topLeft, &pCorners->topLeft, sizeof(s3eLocation));
			//memcpy(&g_pCorners.topRight, &pCorners->bottomLeft, sizeof(s3eLocation));

			g_inverted = false;

			g_rotationAngle = LiveMaps::CalculateSlopeAngleBetweenPoints(&pCorners->topLeft, &pCorners->topRight);
		}

		double rot = g_rotationAngle;

		double scale = MAX(g_xScale, g_yScale);

		if (!g_bStretchToFit)
		{
			g_xScale = g_yScale = scale;
		}

		g_p1To2Distance = LiveMaps::CalculateDistance(&g_pCorners.topLeft, &g_pCorners.topRight);

		//double perCentAngle = LiveMaps::RadToDeg(g_rotationAngle) / 360;
		//double degAng = IW_GEOM_ONE * perCentAngle;
		//iwangle degAng2 = (iwangle)(IW_GEOM_ONE * perCentAngle);

		//CIwMat rotZ;
		//rotZ.SetRotZ(-degAng);
		//g_rotationMatrix.t.z = -0x200;
		//g_rotationMatrix.CopyRot(rotZ);

		g_centerLocation.m_Latitude = (pCorners->topLeft.m_Latitude + pCorners->bottomRight.m_Latitude) / 2;
		g_centerLocation.m_Longitude = (pCorners->topLeft.m_Longitude + pCorners->bottomRight.m_Longitude) / 2;
	}
}

double CoordinateScaler::GetMetersPerPixelX()
{
	return g_metersPerPixelX;
}
double CoordinateScaler::GetMetersPerPixelY()
{
	return g_metersPerPixelY;
}

CIwFVec2 CoordinateScaler::GetSpeedInPixels(CIwFVec2& metersPerSecond)
{
	CIwFVec2 pixelsPerSecond;

	double metPerPixX = GetMetersPerPixelX();
	double metPerPixY = GetMetersPerPixelY();

	double pixelsPerSecondX = metersPerSecond.x / metPerPixX;
	double pixelsPerSecondY = metersPerSecond.y / metPerPixY;

	pixelsPerSecond.x = (float)pixelsPerSecondX;
	pixelsPerSecond.y = (float)pixelsPerSecondY;

	return pixelsPerSecond;
}

CIwFMat CoordinateScaler::GetInverseRotationMatrix(double progress)
{
	if (g_lastProgress == progress)
	{
		return g_invRotationMatrix;
	}

	// Caluclate the matrix and its inverse
	GetRotationMatrix(progress);

	return g_invRotationMatrix;
}

CIwMat CoordinateScaler::GetDrawRotationMatrix(double progress)
{
	GetRotationMatrix(progress);

	return g_drawRotationMatrix;
}

CIwMat CoordinateScaler::GetRotationMatrix(double progress)
{
	if (g_lastProgress == progress)
	{
		return g_rotationMatrix;
	}

	double rotAngle = GetRotationAngle();
	double perCentAngle = -LiveMaps::RadToDeg(rotAngle) / 360;

	perCentAngle = perCentAngle * progress;
	double perCentDrawAngle = -perCentAngle;

	double scaleX = 1 + ((GetScaleX() - 1) * progress);
	double scaleY = 1 + ((GetScaleY() - 1) * progress);

	CIwMat tempDrawMat;
	CIwMat tempMat;
	CIwFMat tempInvMat;
	tempMat.SetIdentity();
	tempMat.t.z = -0x200;

	tempDrawMat.SetIdentity();
	tempDrawMat.t.z = -0x200;
	
	tempInvMat.SetIdentity();
	tempInvMat.t.z = 0x200;

	// Rotate the angle of our view matrix
	iwangle degAng = (iwangle)(IW_GEOM_ONE * perCentAngle);

	// Rotate the angle of our view matrix
	iwangle drawDegAng = (iwangle)(IW_GEOM_ONE * perCentDrawAngle);

	CIwMat drawRotZ;
	CIwMat rotZ;
	CIwFMat rotInvZ;

	drawRotZ.SetRotZ(drawDegAng);
	rotZ.SetRotZ(degAng);
	rotInvZ = rotZ.GetTranspose().ConvertToCIwFMat();

	tempDrawMat.CopyRot(drawRotZ);
	tempMat.CopyRot(rotZ);
	tempInvMat.CopyRot(rotInvZ);

	CIwMat scaleMat;
	CIwFMat scaleInvMat;

	scaleMat.SetIdentity();
	scaleMat.m[0][0] = (iwfixed)(scaleMat.m[0][0] / scaleX);
	scaleMat.m[1][1] = (iwfixed)(scaleMat.m[1][1] / scaleY);

	scaleInvMat.SetIdentity();
	scaleInvMat.m[0][0] = (float)(scaleInvMat.m[0][0] * scaleX);
	scaleInvMat.m[1][1] = (float)(scaleInvMat.m[1][1] * scaleY);

	g_drawRotationMatrix = tempDrawMat.PreMult(scaleMat);
	g_rotationMatrix = tempMat.PreMult(scaleMat);
	g_invRotationMatrix = tempInvMat.PostMult(scaleInvMat);
	g_lastProgress = progress;

	return g_rotationMatrix;
}

GPSRectangle CoordinateScaler::GetScaledCorners()
{
	return g_pScaledCorners;
}

double CoordinateScaler::GetRotationAngle()
{
	return g_rotationAngle;
}

double CoordinateScaler::GetScaleX()
{
	InitScale();
	return g_xScale;
}

double CoordinateScaler::GetScaleY()
{
	InitScale();
	return g_yScale;
}

inline void CoordinateScaler::InitScale()
{
	if (!g_scaleInit && g_pBaseScaler)
	{
		CIwSVec2 topLeft;
		CIwSVec2 topRight;
		CIwSVec2 bottomLeft;
		CIwSVec2 bottomRight;

		// Use our original scaler to get the screen coordinates of each corner
		g_pBaseScaler->LocationToPosition(g_pScaledCorners.topLeft, &topLeft);
		g_pBaseScaler->LocationToPosition(g_pScaledCorners.topRight, &topRight);
		g_pBaseScaler->LocationToPosition(g_pScaledCorners.bottomLeft, &bottomLeft);
		g_pBaseScaler->LocationToPosition(g_pScaledCorners.bottomRight, &bottomRight);
		
		CIwSVec3 topLeft3(topLeft.x - g_width/2, topLeft.y - g_height/2, 0);
		CIwSVec3 topRight3(topRight.x - g_width/2, topRight.y - g_height/2, 0);
		CIwSVec3 bottomLeft3(bottomLeft.x - g_width/2, bottomLeft.y - g_height/2, 0);
		CIwSVec3 bottomRight3(bottomRight.x - g_width/2, bottomRight.y - g_height/2, 0);

		// Create a temporary rotational matrix
		// We will rotate the 4 corners of our grid, then calculate the max distance
		// in the X and Y directions to get the width and height.
		CIwMat rotMatrix;
		rotMatrix.SetIdentity();
		
		double rotAngle = GetRotationAngle();
		double perCentAngle = -LiveMaps::RadToDeg(rotAngle) / 360;
		iwangle degAng = (iwangle)(IW_GEOM_ONE * perCentAngle);

		CIwMat rotZ;
		rotZ.SetRotZ(degAng);
		rotMatrix.CopyRot(rotZ);

		topLeft3 = rotMatrix.RotateVec(topLeft3);
		topRight3 = rotMatrix.RotateVec(topRight3);
		bottomLeft3 = rotMatrix.RotateVec(bottomLeft3);
		bottomRight3 = rotMatrix.RotateVec(bottomRight3);
		
		// Get the max and mix X and Y
		int maxX = MAX4(topLeft3.x, topRight3.x, bottomLeft3.x, bottomRight3.x);
		int minX = MIN4(topLeft3.x, topRight3.x, bottomLeft3.x, bottomRight3.x);
		int maxY = MAX4(topLeft3.y, topRight3.y, bottomLeft3.y, bottomRight3.y);
		int minY = MIN4(topLeft3.y, topRight3.y, bottomLeft3.y, bottomRight3.y);

		// Calculate the scale by the X distance divided by the screen width
		double scaleX = (double)(maxX-minX) / g_width;
		double scaleY = (double)(maxY-minY) / g_height;

		g_yScale = scaleY;
		g_xScale = scaleX;

		g_metersPerPixelX = LiveMaps::MetersPerPixel(g_curZoom) * g_xScale;
		g_metersPerPixelY = LiveMaps::MetersPerPixel(g_curZoom) * g_yScale;

		g_scaleInit = true;
	}
}

double CoordinateScaler::GetCenterLatitude()
{
	return g_centerLocation.m_Latitude;
}

double CoordinateScaler::GetCenterLongitude()
{
	return g_centerLocation.m_Longitude;
}

void CoordinateScaler::PositionToLocation(CIwSVec2& pPosition, s3eLocation* pLocation)
{
	if (!g_bStretchToFit)
	{
		double lon = g_pCorners.topLeft.m_Longitude + (((double)pPosition.x / (g_width)) * (g_pCorners.topRight.m_Longitude - g_pCorners.topLeft.m_Longitude));
		double lat = g_pCorners.topLeft.m_Latitude + (((double)pPosition.y / (g_height)) * (g_pCorners.bottomLeft.m_Latitude - g_pCorners.topLeft.m_Latitude));

		pLocation->m_Latitude = lat;
		pLocation->m_Longitude = lon;
	}
	else
	{
		if (!g_pBaseScaler)
		{
			return;
		}

		// We use our inverse rotational+scale matrix to determine the world coordinates
		// based on the screen coordinates.
		CIwFVec3 screenPosition((float)(pPosition.x - g_width/2) , (float)(pPosition.y - g_height/2), 0);

		CIwFMat rotMatrix = GetInverseRotationMatrix(1.0);

		CIwFVec3 worldPosition = rotMatrix.RotateVec(screenPosition);

		CIwSVec2 screenPosition2D;
		screenPosition2D.x = (int)worldPosition.x + (g_width/2);
		screenPosition2D.y = (int)worldPosition.y + (g_height/2);

		return g_pBaseScaler->PositionToLocation(screenPosition2D, pLocation);
	}
}

void CoordinateScaler::LocationToPosition(s3eLocation& pLocation, CIwSVec2* pPosition)
{
	if (!g_bStretchToFit)
	{
		short x = (short)(g_width * (pLocation.m_Longitude-g_pCorners.topLeft.m_Longitude) / (g_pCorners.topRight.m_Longitude - g_pCorners.topLeft.m_Longitude));
		short y = (short)(g_height * (pLocation.m_Latitude-g_pCorners.topLeft.m_Latitude) / (g_pCorners.bottomLeft.m_Latitude - g_pCorners.topLeft.m_Latitude));

		pPosition->x = x;
		pPosition->y = y;
	}
	else
	{
		if (!g_pBaseScaler)
		{
			return;
		}

		CIwSVec2 position;
		g_pBaseScaler->LocationToPosition(pLocation, &position);

		// We use our rotational+scale matrix to convert world coordinates
		// to screen coordinates.
		CIwVec3 worldPosition((position.x - g_width/2), (position.y - g_height/2), -0x200);
		CIwMat rotMat = GetRotationMatrix(1.0);
		CIwVec3 screenPosition = rotMat.RotateVec(worldPosition);

		pPosition->x = (int16)(screenPosition.x + g_width/2);
		pPosition->y = (int16)(screenPosition.y + g_height/2);
	}
}
