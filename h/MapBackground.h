#pragma once

#include "Main.h"
#include "MapBackground.h"
#include "s3e.h"
#include "s3eLocation.h"
#include "IwHTTPQueue.h"
#include "IwTexture.h"
#include "IwMaterial.h"
#include "IwJpeg.h"
#include "IwImage.h"
#include "Iw2D.h"
#include "IwUI.h"
#include "IwUIImage.h"
#include "IwGxPrint.h"
#include "LiveMaps.h"
#include "CoordinateScaler.h"

#define MAX_TILE_CACHE_COUNT	20

class MapBackground : public CIwUIElement
{
public:
	MapBackground(void);
	~MapBackground(void);

	void Init();
	void ShutDown();

	virtual void Render(CIwUIGraphics& graphics);
	bool Update(bool forceUpdate);
	void DownloadTiles();
	void Render();
	void SetLocation(double longitude, double latitude);
	CoordinateScaler* GetScaler();
	bool IsInitialized();
	bool IsScaled();
	bool IsAnimating();
	double GetLatitudePerPixel();
	double GetLongitudePerPixel();

	CIwSVec2 GetPosition();

	bool UpdatePositionFromMouseAndKeyboard(CIwSVec2& clickPos, bool* pbClicked);

	void SetScaledCorners(s3eLocation& topLeft, s3eLocation& topRight, s3eLocation& bottomLeft, s3eLocation& bottomRight, float zoom);
	void ClearScaledCorners();
	void GetLocation(s3eLocation& pLocation);
	void SetAlpha(double dAlpha);
	double GetZoom() { return g_actualZoom; }

	bool g_bInProgress;

protected:
	virtual void Clone(CIwUIElement* pTarget) const;
	virtual void RenderElement(CIwUIGraphics& graphics);
	virtual CIwUIRect GetElementBounds() const;
	virtual void OnPropertyChanged(const char* pName);
	virtual void OnSizeChanged();
	virtual CIwVec2 MeasureElement(const CIwVec2& availableSize) const;
	virtual void Animate(const CIwUIAnimData& animData);

	bool IsTileVisible(MapTile* pMapTile);
	bool CalculateTiles();
	bool CalculatePosition(bool forceDownload);
	void RenderBackgroundOnSurface(CIw2DSurface* pSurface);
	void RenderBackground();
	MapTile* GetNextDownload(MapTile* pCurrent);
	void AbortTile(MapTile* pTile, bool forceDelete);
	void CreateMapTileImage(MapTile* pMapTile, void* gResult, uint32 gResultLen, bool isJpg);
	void CreateMapTileImage2(MapTile* pMapTile, char* szPath, bool isJpg);
	void LoadMapTileImage(MapTile* pTile);
	void SaveMapTileImage(MapTile* pTile, void* gResult, uint32 gResultLen, bool isJpg);
	void EmptyTileList(std::list<MapTile*>* pAbortList, bool forceDelete);
	void ProcessNewDownloads(std::list<MapTile*>* pNewTiles);
	void OrientCoordinates(s3eLocation* a, s3eLocation* b, s3eLocation* c, s3eLocation* d, int* width, int* height, GPSRectangle& rect);

private:
	static int32 GotHeaders(void* pDownloaderVoid, void* pMapTileVoid);
	static int32 GotData(void* pDownloaderVoid, void* pMapTileVoid);
	static void GotImage(void * Argument, const char* szContentType, const char * Result, uint32 ResultLen);
	static void GotImageError(void * Argument);

	s3eLocation	gPrevLocation;
	s3eLocation	gLocation;
	s3eResult	gError;
	std::list<MapTile*> gVectorImageUrls;
	std::list<MapTile*> gDeleteImageUrls;
	CIw2DImage* gCursor;
	CoordinateScaler* gScaler;
	CoordinateScaler* gScaledModeScaler;
	CIw2DSurface* g_pScaledSurface;
	CIw2DSurface* g_pRotatedSurface;
	CIw2DImage* g_pRotatedSurfaceImage;
	CIw2DImage* g_pScaledSurfaceImage;
	static CIwTexture* g_pNotFoundTexture;
	static CIwTexture* g_pLoadingTexture;
	GPSRectangle g_corners;
	GPSRectangle g_scaledCorners;
	void*		g_pThisAndTile;

	CIwArray<MapTile*> g_pTileCache;
	int g_iTileCacheCount;

	float		g_newZoom;
	float		g_tempZoom;
	float		g_actualZoom;
	float		g_multitouchZoom;
	float		g_curZoom;
	char*		gResult;
	uint32		gResultLen;
	uint8*		pImage;
	int32		g_Width;
	int32		g_Height;
	int			g_cursorIter;

	int g_rows, g_cols;

	bool g_bScaledMode;
	bool g_bInitialLoad;
	bool g_bShowCursor;
	bool g_bMultiTouch;
	bool g_bLocationChanged;

	bool g_bMouseDown;
	CIwSVec2 g_lastMousePos, g_downMousePos;

	bool g_bIsAnimating;
	int g_iAnimationIndex;
	double g_animationPercent;
	CIwMat g_viewMatrix;
	
	double g_latPerPixel, g_lonPerPixel, g_dAlpha;
};

struct ThisAndTile
{
	MapBackground* pThis;
	MapTile* pTile;
};