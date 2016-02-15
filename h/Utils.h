#ifndef UTILS
#define UTILS
#include "IwGx.h"
#include <stdlib.h>
#include <stdio.h>
#include <list>
#include "s3eLocation.h"
#include "s3eKeyboard.h"

typedef void (*DrawPrimsDelegate)(CIwSVec2* pCoords);

class Triangle
{
public:
	~Triangle()
	{
	}

	s3eLocation A,B,C;

	CIwFVec2 CrossProduct(CIwFVec2 & u, CIwFVec2 & v) const 
	{
		return CIwFVec2(u.x * v.y, u.y * -v.x); 
	} 

	bool SameSide(CIwFVec2& p1, CIwFVec2& p2, CIwFVec2& a, CIwFVec2& b)
	{
		CIwFVec2 bMa = (b-a);
		CIwFVec2 p1Ma = (p1-a);
		CIwFVec2 p2Ma = (p2-a);

		CIwFVec2 cp1 = CrossProduct(bMa, p1Ma);
		CIwFVec2 cp2 = CrossProduct(bMa, p2Ma);

		if ((cp1 * cp2) >= 0)
		{
			return true;
		}
		return false;
	}

	bool Contains(s3eLocation& compare)
	{
		// We draw a line straight up (or sideways). If it intersects with exactly 1 line, we are inside the triangle.
		// If it hits 2, we are above or below
		// If it hits, 0, we are outside
		CIwFVec2 colPt;
		iwfixed t1, t2;

		CIwFVec2 sA(A.m_Longitude, A.m_Latitude);
		CIwFVec2 sB(B.m_Longitude, B.m_Latitude);
		CIwFVec2 sC(C.m_Longitude, C.m_Latitude);
		CIwFVec2 sComp(compare.m_Longitude, compare.m_Latitude);

		// Compute vectors        
		CIwFVec2 v0 = sC - sA;
		CIwFVec2 v1 = sB - sA;
		CIwFVec2 v2 = sComp - sA;

		// Compute dot products
		float dot00 = (v0 * v0);
		float dot01 = (v0 * v1);
		float dot02 = (v0 * v2);
		float dot11 = (v1 * v1);
		float dot12 = (v1 * v2);

		// Compute barycentric coordinates
		float invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
		float u = (dot11 * dot02 - dot01 * dot12) * invDenom;
		float v = (dot00 * dot12 - dot01 * dot02) * invDenom;

		// Check if point is in triangle
		if ((u > 0) && (v > 0) && (u + v < 1))
		{
			return true;
		}
		return false;


		if (SameSide(sComp, sA, sB, sC) & SameSide(sComp, sB, sA, sC) && SameSide(sComp, sC, sA, sB))
		{
			return true;
		}
		return false;


/*
		CIwVec2 sCompEnd(0, 10000);

		bool intAB = IwIntersectLineLine2D(sComp, sCompEnd, sA, (sB - sA), colPt, t1, t1);
		bool intBC = IwIntersectLineLine2D(sComp, sCompEnd, sB, (sC - sB), colPt, t1, t1);
		bool intCA = IwIntersectLineLine2D(sComp, sCompEnd, sC, (sA - sC), colPt, t1, t1);

		int collisions = 0;
		if (intAB)
		{
			collisions++;
		}
		if (intBC)
		{
			collisions++;
		}
		if (intCA)
		{
			collisions++;
		}

		return (collisions == 2);
		*/
	}
};

class Region
{
public:
	~Region()
	{
		Clear();
	}
	void SetBoundingBox(s3eLocation& TL, s3eLocation& BR)
	{
		g_tlBB.m_Longitude = TL.m_Longitude;
		g_tlBB.m_Latitude = TL.m_Latitude;
		g_brBB.m_Longitude = BR.m_Longitude;
		g_brBB.m_Latitude = BR.m_Latitude;
		
		if (g_tlBB.m_Longitude > g_brBB.m_Longitude)
		{
			float longitude = g_brBB.m_Longitude;
			g_brBB.m_Longitude = g_tlBB.m_Longitude;
			g_tlBB.m_Longitude = longitude;
		}
		if (g_tlBB.m_Latitude > g_brBB.m_Latitude)
		{
			float latitude = g_brBB.m_Latitude;
			g_brBB.m_Latitude = g_tlBB.m_Latitude;
			g_tlBB.m_Latitude = latitude;
		}

		g_midBB.m_Longitude = (g_tlBB.m_Longitude + g_brBB.m_Longitude) / 2;
		g_midBB.m_Latitude = (g_tlBB.m_Latitude + g_brBB.m_Latitude) / 2;
	}
	void Add(s3eLocation& A, s3eLocation& B)
	{
		Triangle tri;
		tri.A = A;
		tri.B = B;
		tri.C = g_midBB;

		g_triangles.append(tri);
	}
	bool Contains(s3eLocation& compare)
	{
		s3eLocation tl = g_tlBB;
		s3eLocation br = g_brBB;
		if (compare.m_Longitude >= g_tlBB.m_Longitude && compare.m_Latitude >= g_tlBB.m_Latitude)
		{
			if (compare.m_Longitude <= g_brBB.m_Longitude && compare.m_Latitude <= g_brBB.m_Latitude)
			{
				// We are contained in the bounding box...
				// Check if we are in one of the triangles
				for (int i = 0; i < g_triangles.size(); ++i)
				{
					if (g_triangles[i].Contains(compare))
					{
						return true;
					}
				}
			}
		}
		return false;
	}
	void Clear()
	{
		for (int i = 0; i < g_triangles.size(); ++i)
		{
			//delete pTri;
		}
		g_triangles.clear();
	}
public :
	CIwArray<Triangle> g_triangles;
	s3eLocation g_tlBB, g_brBB, g_midBB;
};

class Utils
{
public:
	static void DrawPrimsQuadList(CIwSVec2* pCoords);
	static void AlphaRenderImage(CIwTexture* pTexture, CIwSVec2& location, double alpha);
	static void AlphaRenderImage(CIwTexture* pTexture, CIwSVec2& location, double alpha, DrawPrimsDelegate pfnDrawPrims);
	static void AlphaRenderImage(CIwTexture* pTexture, CIwRect& bounds, double alpha);
	static void AlphaRenderImage(CIwTexture* pTexture, CIwRect& bounds, double alpha, DrawPrimsDelegate pfnDrawPrims);
	static void AlphaRenderAndRotateImage(CIwTexture* pTexture, CIwRect& bounds, double alpha, double rotationAngle);

	static bool LoadMap(const char* szData, CIwArray<s3eLocation>* pPoints, float* pZoom);
	static char* GetMapData(const char* szMap);
	static void SaveMapData(const char* szMap, const char* szData);
	static void LoadRegion(Region& r, CIwArray<s3eLocation>* pPoints);

	static void DownloadMapTile(CIwTexture** ppTile, char* szImageUrl);
	static void GotTile(void * Argument, const char* szContentType, const char * Result, uint32 ResultLen);
	static void GotTileError(void * Argument);

	static void ResetLocation()
	{
		g_lonOffset = g_latOffset = 0;
	}
	static void UpdateLocation()
	{
		if (s3eKeyboardGetState(s3eKeyRight) & S3E_KEY_STATE_DOWN)
		{
			g_lonOffset += .00001;
		}
		if (s3eKeyboardGetState(s3eKeyLeft) & S3E_KEY_STATE_DOWN)
		{
			g_lonOffset -= .00001;
		}
		if (s3eKeyboardGetState(s3eKeyDown) & S3E_KEY_STATE_DOWN)
		{
			g_latOffset += .00001;
		}
		if (s3eKeyboardGetState(s3eKeyUp) & S3E_KEY_STATE_DOWN)
		{
			g_latOffset -= .00001;
		}
	}
	static void GetLocation(s3eLocation* pLocation)
	{
		// TODO: cap the frequency we call this
		uint64 time = s3eTimerGetMs();

		if ((time - g_lastGpsRead) > 500)
		{
			g_lastGpsRead = time;
			s3eLocationGet(&g_lastGpsCoord);
		}
		*pLocation = g_lastGpsCoord;
		
		pLocation->m_Latitude += g_latOffset;
		pLocation->m_Longitude += g_lonOffset;
	}

	static s3eLocation g_lastGpsCoord;
	static uint64 g_lastGpsRead;
	static float g_latOffset, g_lonOffset;
};

#endif

