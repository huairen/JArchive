#include "JTypes.h"
#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")

class JGDIHelper
{
public:
	static Gdiplus::Image* LoadImageFormResource(HINSTANCE hInst, int nResID);
	static void DrawAlphaImage(Gdiplus::Image* pImage, HWND hWnd, HDC hDc, int x, int y);
};